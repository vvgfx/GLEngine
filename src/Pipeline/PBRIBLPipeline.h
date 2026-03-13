#ifndef _PBRIBLPIPELINE_H_
#define _PBRIBLPIPELINE_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "AbstractPipeline.h"
#include "../sgraph/IScenegraph.h"
#include <ShaderProgram.h>
#include <ShaderGeoProgram.h>
#include <ShaderLocationsVault.h>
#include "../sgraph/SGNodeVisitor.h"
#include <ObjectInstance.h>
#include <Light.h>
#include "LightLocation.h"
#include <VertexAttrib.h>
#include <TextureImage.h>
#include "../sgraph/TexturedPBRRenderer.h"
#include "../sgraph/LightRetriever.h"
#include "TangentComputer.h"
#include "stb_image.h"

// temporary imports

namespace pipeline
{
    /**
     * An implementation the pipeline interface. This pipeline features lights (directional and spotlights), textures, PBR workflow and IBL.
     * Note that this pipeline REQUIRES PBR textures to be defined to work properly.
     * To use this pipeline, initalize it using init() and draw a single frame using drawFrame()
     */
    class PBRIBLPipeline : public AbstractPipeline
    {

    public:
        inline void init(map<string, util::PolygonMesh<VertexAttrib>>& meshes, glm::mat4 &projection, map<string, unsigned int>& texMap);
        inline void addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh);
        inline void drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat);
        inline void initLights(sgraph::IScenegraph *scenegraph);
        inline void initShaderVars();
        inline void loadCubeMap(vector<util::TextureImage*>& cubeMap) override;
        inline void drawCubeMap(glm::mat4 &viewMat) override;

    private:
        util::ShaderProgram shaderProgram;
        util::ShaderProgram hdrSkyboxShaderProgram;
        util::ShaderLocationsVault shaderLocations;
        util::ShaderLocationsVault hdrSkyboxShaderLocations;
        sgraph::SGNodeVisitor *renderer;
        sgraph::SGNodeVisitor *lightRetriever;
        map<string, unsigned int>* textureIdMap;
        vector<util::Light> lights;
        vector<glm::mat4> lightTransformations;
        std::map<string, sgraph::TransformNode *> cachedNodes;
        vector<LightLocation> lightLocations;
        bool initialized = false;
        int frames;
        double time;

        map<string, string> shaderVarsToVertexAttribs;

        bool hdrCubemap = false;

        // HDR cubemap stuff here
        unsigned int captureFBO, captureRBO, hdrSkyboxTexture, envCubemap;
        int skyboxWidth, skyboxHeight;

        // irradiance map stuff here
        unsigned int irradianceMap;

        // specular stuff here
        unsigned int prefilterMap, brdfLUTTexture;
    };

    void PBRIBLPipeline::init(map<string, util::PolygonMesh<VertexAttrib>>& meshes, glm::mat4 &proj, map<string, unsigned int>& texMap)
    {
        this->projection = proj;
        shaderProgram.createProgram("shaders/PBR/TexturePBR-IBL.vert",
                                    "shaders/PBR/TexturePBR-IBL.frag");
        shaderProgram.enable();
        shaderLocations = shaderProgram.getAllShaderVariables();
        shaderProgram.disable();

        hdrSkyboxShaderProgram.createProgram("shaders/cubemap/HDR/hdrSkyboxToneMapping.vert",
                                             "shaders/cubemap/HDR/hdrSkyboxToneMapping.frag");

        hdrSkyboxShaderProgram.enable();
        hdrSkyboxShaderLocations = hdrSkyboxShaderProgram.getAllShaderVariables();
        hdrSkyboxShaderProgram.disable();

        // Mapping of shader variables to vertex attributes
        shaderVarsToVertexAttribs["vPosition"] = "position";
        shaderVarsToVertexAttribs["vNormal"] = "normal";
        shaderVarsToVertexAttribs["vTexCoord"] = "texcoord";
        shaderVarsToVertexAttribs["vTangent"] = "tangent";

        textureIdMap = &texMap;

        for (typename map<string, util::PolygonMesh<VertexAttrib>>::iterator it = meshes.begin();
             it != meshes.end();
             it++)
        {
            util::ObjectInstance *obj = new util::ObjectInstance(it->first);
            TangentComputer::computeTangents(it->second); // compute tangents
            obj->initPolygonMesh(shaderLocations, shaderVarsToVertexAttribs, it->second);
            objects[it->first] = obj;
        }
        renderer = new sgraph::TexturedPBRRenderer(modelview, objects, shaderLocations, *textureIdMap);
        lightRetriever = new sgraph::LightRetriever(modelview);

        initialized = true;
    }


    /**
     * Need to override this because this should support hdr images and the implementation is too specific!
     */
    void PBRIBLPipeline::loadCubeMap(vector<util::TextureImage*>& cubeMap)
    {
        if(cubeMap.size() > 1)  // ldr
        {
            AbstractPipeline::loadCubeMap(cubeMap);
            return;
        }
        cout<<"Loading HDR cubemap"<<endl;
        if (!hasObject("hdr-skybox")) {
            cerr << "Warning: 'hdr-skybox' mesh not found, skipping HDR IBL setup." << endl;
            return;
        }
        if (!hasObject("postProcess")) {
            cerr << "Warning: 'postProcess' mesh not found, skipping HDR IBL setup (needed for BRDF LUT)." << endl;
            return;
        }
        hdrCubemap = true;
        // draw the cubemap and save to memory.
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        // generate new framebuffer.
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);// format for the render buffer.
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO); // use this render buffer as an attachment.

        util::ShaderProgram equiRectangularShader;
        equiRectangularShader.createProgram("shaders/cubemap/HDR/equiRectangular.vert", 
                                            "shaders/cubemap/HDR/equiRectangular.frag"); // create these!!

        // get shader locations
        equiRectangularShader.enable();
        util::ShaderLocationsVault equiRectShaderLocations = equiRectangularShader.getAllShaderVariables();
        equiRectangularShader.disable();


        float *data = cubeMap[0]->getFloatImage();
        skyboxWidth = cubeMap[0]->getWidth();
        skyboxHeight = cubeMap[0]->getHeight();
        glGenTextures(1, &hdrSkyboxTexture);
        glBindTexture(GL_TEXTURE_2D, hdrSkyboxTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, skyboxWidth, skyboxHeight, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // set up the cubemap
        glGenTextures(1, &envCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        for(unsigned int i = 0; i < 6; i++)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // set up the views for each face.
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        equiRectangularShader.enable();
        glUniform1i(equiRectShaderLocations.getLocation("equirectangularMap"), 0); // use TEXTURE0
        glUniformMatrix4fv(equiRectShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrSkyboxTexture);

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(equiRectShaderLocations.getLocation("view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            objects["hdr-skybox"]->draw();
        }
        equiRectangularShader.disable();

        // create mipmaps to stop visible dot artifacts from occuring.
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // -------------------------------------------------------------------------------------------------------------
        // At this point, we have the hdr texture mapped to a cubemap. Now we need to build the irradiance map for the diffuse part.
        cout<<"Building diffuse irradiance cubemap!"<<endl;
        util::ShaderProgram irradianceShaderProgram;
        irradianceShaderProgram.createProgram("shaders/cubemap/HDR/irradiance.vert",
                                            "shaders/cubemap/HDR/irradiance.frag");
        util::ShaderLocationsVault irradianceShaderLocations;
        irradianceShaderProgram.enable();
        irradianceShaderLocations = irradianceShaderProgram.getAllShaderVariables();
        irradianceShaderProgram.disable();

        glGenTextures(1, &irradianceMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        // set all sides as GL_RGB16F
        for(unsigned int i = 0; i < 6; i++)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr); // note : each side is of size 32

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // reuse the same framebuffer!
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32); // same size 32 here!

        irradianceShaderProgram.enable();
        glUniform1i(irradianceShaderLocations.getLocation("environmentMap"), 0); // send environment map as TEXTURE0
        glUniformMatrix4fv(irradianceShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        glViewport(0, 0, 32, 32); // once again, 32 here!
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO); // this shouldn't be required, but playing it safe!
        for(unsigned int i = 0; i < 6; i++)
        {
            glUniformMatrix4fv(equiRectShaderLocations.getLocation("view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            objects["hdr-skybox"]->draw();
        }

        irradianceShaderProgram.disable();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //---------------------------------------------------------------------------------------------------------------
        // Moving on to specular highlights.
        // Creating the pre-filter cubemap for the first half of the split-sum approximation.
        cout<<"Building specular pre-filter map!"<<endl;
        glGenTextures(1, &prefilterMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
         for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
        
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        //------------------------------------------------------------------------------------------
        // run quasi monte-carlo simulation on environment lighting to create the pre-filter cubemap
        util::ShaderProgram prefilterShaderProgram;
        prefilterShaderProgram.createProgram("shaders/cubemap/HDR/prefilter.vert",
                                    "shaders/cubemap/HDR/prefilter.frag");
        util::ShaderLocationsVault prefilterShaderLocations;
        prefilterShaderProgram.enable();
        prefilterShaderLocations  = prefilterShaderProgram.getAllShaderVariables();
        prefilterShaderProgram.disable();

        prefilterShaderProgram.enable();
        glUniformMatrix4fv(prefilterShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glActiveTexture(GL_TEXTURE0);  // using texture 0 as the environment map.
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glUniform1i(prefilterShaderLocations.getLocation("environmentMap"), 0);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        unsigned int maxMipLevels =   5;
        for(unsigned int mip  = 0; mip < maxMipLevels; mip++)
        {
            unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            glUniform1f(prefilterShaderLocations.getLocation("roughness"), roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                glUniformMatrix4fv(prefilterShaderLocations.getLocation("view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                objects["hdr-skybox"]->draw();
            }
        }

        prefilterShaderProgram.disable();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //------------------------------------------------------------------------------------------
        // generate 2d LUT from BRDF.
        cout<<"Building LUT from BRDF!"<<endl;
        util::ShaderProgram brdfShader;
        brdfShader.createProgram("shaders/cubemap/HDR/brdf.vert", 
                                "shaders/cubemap/HDR/brdf.frag");

        util::ShaderLocationsVault brdfShaderLocations;

        brdfShader.enable();
        brdfShaderLocations = brdfShader.getAllShaderVariables();
        brdfShader.disable();


        
        brdfShader.enable();
        glGenTextures(1, &brdfLUTTexture);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // once again, using the same framebuffer xD

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

        glViewport(0, 0, 512, 512);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objects["postProcess"]->draw();
        

        brdfShader.disable();
        // reset to original viewport!
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        cubeMapLoaded = true;
    }

    void PBRIBLPipeline::drawCubeMap(glm::mat4 &viewMat)
    {
        if(!hdrCubemap)
        {
            AbstractPipeline::drawCubeMap(viewMat);
            return;
        }
        hdrSkyboxShaderProgram.enable();
        glUniformMatrix4fv(hdrSkyboxShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(hdrSkyboxShaderLocations.getLocation("view"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glUniform1i(hdrSkyboxShaderLocations.getLocation("environmentMap"), 0);
        if (hasObject("hdr-skybox"))
            objects["hdr-skybox"]->draw();
        hdrSkyboxShaderProgram.disable();
    }

    void PBRIBLPipeline::addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh)
    {
        TangentComputer::computeTangents(mesh);
        util::ObjectInstance *obj = new util::ObjectInstance(objectName);
        obj->initPolygonMesh(shaderLocations, shaderVarsToVertexAttribs, mesh);
        objects[objectName] = obj;
    }

    void PBRIBLPipeline::drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        if (!initialized)
            throw runtime_error("pipeline has not been initialized.");

        if(!cubeMapLoaded)
            throw runtime_error("skybox has not been provided! This pipeline requires a skybox");
        // can't pass the view vec3 directly so doing this.
        glm::mat4 inverseView = glm::inverse(viewMat);
        cameraPos = glm::vec3(inverseView[3]);

        shaderProgram.enable();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        modelview.push(glm::mat4(1.0f));
        // modelview.top() = modelview.top() * viewMat; // This means all the lights will be in the view coordinate system. - Commented out because all transformations are in the world coordinate system now
        initLights(scenegraph); // lighting scenegraph traversal happens here. I've moved this to the first because the lights need to be initialized
        initShaderVars();
        modelview.pop();

        // passing the camera location to the fragment shader.
        glUniform3fv(shaderLocations.getLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniformMatrix4fv(shaderLocations.getLocation("view"), 1, GL_FALSE, glm::value_ptr(viewMat)); // view transformation

        modelview.push(glm::mat4(1.0));
        // modelview.top() = modelview.top() * viewMat; // -> needs to be in the world coordinate system!
        glUniform1i(shaderLocations.getLocation("numLights"), lights.size());
        for (int i = 0; i < lights.size(); i++)
        {
            glm::vec4 pos = lights[i].getPosition();
            pos = lightTransformations[i] * pos; // world coordinate system.
            glm::vec4 spotDirection = lights[i].getSpotDirection();
            spotDirection = lightTransformations[i] * spotDirection;
            // Set light colors
            glUniform3fv(lightLocations[i].color, 1, glm::value_ptr(lights[i].getColor()));
            glUniform4fv(lightLocations[i].position, 1, glm::value_ptr(pos));
            // spotlight stuff here
            glUniform1f(lightLocations[i].spotAngle, cos(glm::radians(lights[i].getSpotCutoff())));
            glUniform3fv(lightLocations[i].spotDirection, 1, glm::value_ptr(spotDirection));
        }

        glUniformMatrix4fv(shaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // pass the irradiance map!
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        glUniform1i(shaderLocations.getLocation("irradianceMap"), 8);

        // pass the 
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        glUniform1i(shaderLocations.getLocation("prefilterMap"), 7);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        glUniform1i(shaderLocations.getLocation("brdfLUT"), 6);

        scenegraph->getRoot()->accept(renderer);

        if(cubeMapLoaded)
            drawCubeMap(viewMat);

        // test hdr cubemap
        // cout<<"Errors: "<<glGetError()<<endl;
        modelview.pop();
        glFlush();
        shaderProgram.disable();
    }

    void PBRIBLPipeline::initLights(sgraph::IScenegraph *scenegraph)
    {
        sgraph::LightRetriever *lightsParser = reinterpret_cast<sgraph::LightRetriever *>(lightRetriever);
        lightsParser->clearData();
        scenegraph->getRoot()->accept(lightRetriever);
        lights = lightsParser->getLights();
        lightTransformations = lightsParser->getLightTransformations();
    }

    void PBRIBLPipeline::initShaderVars()
    {
        lightLocations.clear();
        for (int i = 0; i < lights.size(); i++)
        {
            LightLocation ll;
            stringstream name;

            name << "light[" << i << "]";
            ll.position = shaderLocations.getLocation(name.str() + "" + ".position");
            ll.color = shaderLocations.getLocation(name.str() + ".color");
            // adding spotDirection and spotAngle.
            ll.spotDirection = shaderLocations.getLocation(name.str() + ".spotDirection");
            ll.spotAngle = shaderLocations.getLocation(name.str() + ".spotAngleCosine");
            lightLocations.push_back(ll);
        }
    }
}

#endif