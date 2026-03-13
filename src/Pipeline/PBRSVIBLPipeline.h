#ifndef _PBRSVIBLPIPELINE_H_
#define _PBRSVIBLPIPELINE_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
#include "../sgraph/TexturedPBRSVRenderer.h"
#include "../sgraph/LightRetriever.h"
#include "../sgraph/ShadowRenderer.h"
#include "../sgraph/DepthRenderer.h"
#include "../sgraph/PBRSVIBLAmbientRenderer.h"
#include "TangentComputer.h"
#include <iostream>

#include "../sgraph/STBImageLoader.h"

namespace pipeline
{
    /**
     * An implementation the pipeline interface. This pipeline features lights (directional and spotlights), shadows using shadow volumes, PBR workflow and IBL
     * Note that this pipeline REQUIRES PBR materials to be defined to work properly, and supports (and requires) textures.
     * To use this pipeline, initalize it using init() and draw a single frame using drawFrame()
     */
    class PBRSVIBLPipeline : public AbstractPipeline
    {

    public:
        inline void init(map<string, util::PolygonMesh<VertexAttrib>> &meshes, glm::mat4 &projection, map<string, unsigned int>& texMap);
        inline void addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh);
        inline void drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat);
        inline void initLights(sgraph::IScenegraph *scenegraph);
        inline void depthPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat);
        inline void shadowStencilPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat, int lightIndex);
        inline void renderObjectPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat, int lightIndex);
        inline void ambientPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat);
        inline void updateProjection(glm::mat4& newProjection);

        // IBL stuff
        inline void loadCubeMap(vector<util::TextureImage*>& cubeMap) override;
        inline void drawCubeMap(glm::mat4 &viewMat) override;

        bool hdrCubemap = false;

        // HDR cubemap stuff here
        unsigned int captureFBO, captureRBO, hdrSkyboxTexture, envCubemap;
        int skyboxWidth, skyboxHeight;

        // irradiance map stuff here
        unsigned int irradianceMap;

        // specular stuff here
        unsigned int prefilterMap, brdfLUTTexture;

    private:
        
        util::ShaderProgram renderProgram;
        util::ShaderProgram depthProgram;
        util::ShaderProgram ambientProgram;
        util::ShaderProgram hdrShaderProgram;
        util::ShaderGeoProgram shadowProgram;
        util::ShaderProgram hdrSkyboxShaderProgram;

        
        util::ShaderLocationsVault renderShaderLocations;
        util::ShaderLocationsVault depthShaderLocations;
        util::ShaderLocationsVault ambientShaderLocations;
        util::ShaderLocationsVault shadowShaderLocations;
        util::ShaderLocationsVault hdrShaderLocations;
        util::ShaderLocationsVault hdrSkyboxShaderLocations;

        sgraph::SGNodeVisitor *renderer;
        sgraph::SGNodeVisitor *lightRetriever;
        sgraph::SGNodeVisitor *shadowRenderer;
        sgraph::SGNodeVisitor *depthRenderer;
        sgraph::SGNodeVisitor *ambientRenderer;

        map<string, unsigned int>* textureIdMap;
        vector<util::Light> lights;
        vector<glm::mat4> lightTransformations;
        
        
        std::map<string, sgraph::TransformNode *> cachedNodes;
        vector<LightLocation> lightLocations;
        
        bool initialized = false;
        int frames;
        double time;
        map<string, string> shaderVarsToVertexAttribs;

        // hdr stuff
        unsigned int hdrFBO, hdrColorBuffer, depthStencilBuffer;

        unsigned int quadVAO = 0;
        unsigned int quadVBO;


    };

    void PBRSVIBLPipeline::init(map<string, util::PolygonMesh<VertexAttrib>> &meshes, glm::mat4 &proj, map<string, unsigned int>& texMap)
    {
        this->projection = proj;

        textureIdMap = &texMap;
        // Render program initialization
        renderProgram.createProgram("shaders/shadow/TexturePBR-SV.vert",
                                    "shaders/shadow/TexturePBR-SV.frag");
        renderProgram.enable();
        renderShaderLocations = renderProgram.getAllShaderVariables();
        renderProgram.disable();

        // Depth program initialization
        depthProgram.createProgram("shaders/shadow/depth.vert",
                                   "shaders/shadow/depth.frag");
        depthProgram.enable();
        depthShaderLocations = depthProgram.getAllShaderVariables();
        depthProgram.disable();

        // Ambient program initialization
        ambientProgram.createProgram("shaders/shadow/Texture-PBR-IBL-ambient.vert",
                                     "shaders/shadow/Texture-PBR-IBL-ambient.frag");
        ambientProgram.enable();
        ambientShaderLocations = ambientProgram.getAllShaderVariables();
        ambientProgram.disable();

        // Shadow program initialization
        shadowProgram.createProgram("shaders/shadow/shadow.vert",
                                    "shaders/shadow/shadow.frag",
                                    "shaders/shadow/shadow.geom");
        shadowProgram.enable();
        shadowShaderLocations = shadowProgram.getAllShaderVariables();
        shadowProgram.disable();

        // hdr shader initialization

        hdrShaderProgram.createProgram( "shaders/postprocessing/hdr.vert",
                                        "shaders/postprocessing/hdr.frag");

        hdrShaderProgram.enable();
        hdrShaderLocations = hdrShaderProgram.getAllShaderVariables();
        hdrShaderProgram.disable();

        // hdr skybox initialization

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

        for (typename map<string, util::PolygonMesh<VertexAttrib>>::iterator it = meshes.begin();
             it != meshes.end();
             it++)
        {
            util::ObjectInstance *obj = new util::ObjectInstance(it->first);
            TangentComputer::computeTangents(it->second);
            obj->initPolygonMesh(renderShaderLocations, shaderVarsToVertexAttribs, it->second);
            objects[it->first] = obj;
        }

        renderer = new sgraph::TexturedPBRSVRenderer(modelview, objects, renderShaderLocations, *textureIdMap);
        lightRetriever = new sgraph::LightRetriever(modelview);
        shadowRenderer = new sgraph::ShadowRenderer(modelview, objects, shadowShaderLocations);
        depthRenderer = new sgraph::DepthRenderer(modelview, objects, depthShaderLocations);
        ambientRenderer = new sgraph::PBRSVIBLAmbientRenderer(modelview, objects, ambientShaderLocations, *textureIdMap);
        initialized = true;

        // getting the screen dimensions from the viewport!
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        // width is the 2nd index, height is the 3rd index

        // create framebuffers for HDR here.
        glGenFramebuffers(1, &hdrFBO);

        // color buffer.
        glGenTextures(1, &hdrColorBuffer);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, viewport[2], viewport[3], 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // combined depth+stencil render buffer
        glGenRenderbuffers(1, &depthStencilBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, viewport[2], viewport[3]);

        // attach to custom framebuffer now.
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0); // sets color texture as color attachment
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilBuffer);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw runtime_error("custom framebuffer is not complete!");
        
        // finally set the default framebuffer back
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    void PBRSVIBLPipeline::loadCubeMap(vector<util::TextureImage*>& cubeMap)
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

    void PBRSVIBLPipeline::drawCubeMap(glm::mat4 &viewMat)
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
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);
        if (hasObject("hdr-skybox"))
            objects["hdr-skybox"]->draw();
        hdrSkyboxShaderProgram.disable();
    }


    void PBRSVIBLPipeline::addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh)
    {
        TangentComputer::computeTangents(mesh);
        util::ObjectInstance *obj = new util::ObjectInstance(objectName);
        obj->initPolygonMesh(renderShaderLocations, shaderVarsToVertexAttribs, mesh);
        objects[objectName] = obj;
    }

    void PBRSVIBLPipeline::drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        if (!initialized)
            throw runtime_error("pipeline has not been initialized.");
        
        if(!cubeMapLoaded)
            throw runtime_error("skybox has not been provided! This pipeline requires a skybox");

        // set current frame buffer as HDR buffer
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

        // Light traversal
        modelview.push(glm::mat4(1.0f));
        modelview.top() = modelview.top() * viewMat;
        initLights(scenegraph); // lighting scenegraph traversal happens here.
        modelview.pop();
        glDepthFunc(GL_LEQUAL);

        // shadow volume pipeline starts here.
        glClearColor(0, 0, 0, 1);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);                                                       // enable writing to the depth buffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // clear everything before starting the render loop.

        depthPass(scenegraph, viewMat); // set the depth buffer from the actual camera location to set up for the stencil test.

        glEnable(GL_STENCIL_TEST);   // enable stencil test.
        glEnable(GL_BLEND);          // for multiple lights
        glBlendFunc(GL_ONE, GL_ONE); //  Equally blend all the effects from eall the lights (is this correct?)
        for (int i = 0; i < lights.size(); i++)
        {
            glClear(GL_STENCIL_BUFFER_BIT);
            shadowStencilPass(scenegraph, viewMat, i); // render the shadow volume into the stencil buffer.
            renderObjectPass(scenegraph, viewMat, i);  // render all the objects with lighting (except ambient) into the scene. (fragments that fail the stencil test will not touch the fragment shader).
        }
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);       // need to disable the stencil test for the ambient pass because all objects require ambient lighting.
        ambientPass(scenegraph, viewMat); // ambient pass for all objects.
        // Note to self: In order to do postprocessing, I might need to write the output to a different framebuffer and then read that as a texture to my post-processing pass -  DONE
        // cout<<"Errors :"<<glGetError()<<endl;
        if(cubeMapLoaded)
            drawCubeMap(viewMat);
        
        // swap to default buffer, with the old color-buffer as input.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        hdrShaderProgram.enable();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
        glUniform1i(hdrShaderLocations.getLocation("hdrColorBuffer"), 0);
        // set exposure here later.
        float exposure = 1.0f;
        glUniform1f(hdrShaderLocations.getLocation("exposure"), exposure);
        // glDisable(GL_DEPTH_TEST); // not sure why this is needed?
        // draw screen space quad
        if (hasObject("postProcess"))
            objects["postProcess"]->draw();
        else
            cerr << "Warning: 'postProcess' mesh not found, skipping HDR tone-mapping pass." << endl;
        hdrShaderProgram.disable();

    }

    void PBRSVIBLPipeline::initLights(sgraph::IScenegraph *scenegraph)
    {
        sgraph::LightRetriever *lightsParser = reinterpret_cast<sgraph::LightRetriever *>(lightRetriever);
        lightsParser->clearData();
        scenegraph->getRoot()->accept(lightRetriever);
        lights = lightsParser->getLights();
        lightTransformations = lightsParser->getLightTransformations();
    }

    void PBRSVIBLPipeline::depthPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        glDrawBuffer(GL_NONE); // Don't want to draw anything, only fill the depth buffer.
        depthProgram.enable();
        modelview.push(glm::mat4(1.0));
        modelview.top() = modelview.top() * viewMat;
        glUniformMatrix4fv(depthShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
        // the modelview will be passed by the renderer (hopefully)
        scenegraph->getRoot()->accept(depthRenderer);
        modelview.pop();
        depthProgram.disable();
        // glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }

    void PBRSVIBLPipeline::shadowStencilPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat, int lightIndex)
    {
        glDrawBuffer(GL_NONE);
        glDepthMask(GL_FALSE);             // do not write into the depth buffer anymore. This is so that the shadow volumes do not obstruct the actual objects.
        glEnable(GL_DEPTH_CLAMP);          // Don't want to clip the back polygons.
        glDisable(GL_CULL_FACE);           // Don't want the back-facing polygons to get culled. need them to increment the stencil buffer.
        glStencilFunc(GL_ALWAYS, 0, 0xff); // Always pass the stencil test, reference value 0,mask value 1(should probably use ~0)

        // These are for depth pass. This works for now, but will fail when the camera is placed inside a shadow.
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);

        // I think I can swap the incrememnt and decrement and I'll still be fine.

        // Now render the scene
        shadowProgram.enable();
        modelview.push(glm::mat4(1.0));
        modelview.top() = modelview.top() * viewMat;
        glUniformMatrix4fv(shadowShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // did the light traversal at the first now.
        glm::vec4 pos = lights[lightIndex].getPosition();
        pos = lightTransformations[lightIndex] * pos;
        // remember that all the lightlocations are in the view coordinate system.
        glm::vec3 sendingVal = glm::vec3(pos);
        glUniform3fv(shadowShaderLocations.getLocation("gLightPos"), 1, glm::value_ptr(sendingVal));
        scenegraph->getRoot()->accept(shadowRenderer);

        modelview.pop();
        shadowProgram.disable();

        // restore original settings now
        glDisable(GL_DEPTH_CLAMP);
        glEnable(GL_CULL_FACE); // need to enable culling so that the next pass doesn't render all faces.
        glDepthMask(GL_TRUE);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }

    void PBRSVIBLPipeline::ambientPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        ambientProgram.enable();
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);  
        modelview.push(glm::mat4(1.0));
        // modelview.top() = modelview.top() * viewMat;  // don't use the view matrix for the ambient pass as the values are required to be in the world space.
        glUniformMatrix4fv(ambientShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(ambientShaderLocations.getLocation("view"), 1, GL_FALSE, glm::value_ptr(viewMat));

        glm::mat4 inverseView = glm::inverse(viewMat);
        glm::vec3 cameraPos = glm::vec3(inverseView[3]);
        glUniform3fv(ambientShaderLocations.getLocation("cameraPos"), 1, glm::value_ptr(cameraPos));

        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        glUniform1i(ambientShaderLocations.getLocation("irradianceMap"), 8);

        // pass the 
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        glUniform1i(ambientShaderLocations.getLocation("prefilterMap"), 7);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        glUniform1i(ambientShaderLocations.getLocation("brdfLUT"), 6);

        scenegraph->getRoot()->accept(ambientRenderer);
        modelview.pop();
        ambientProgram.disable();
        glDisable(GL_BLEND);
    }

    void PBRSVIBLPipeline::renderObjectPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat, int lightIndex)
    {

        renderProgram.enable();
        glDrawBuffer(GL_COLOR_ATTACHMENT0); // enable writing to the color buffer. This was disabled earlier. Update - change this to color-attachment because I'm using PBR with HDR now.
        glStencilFunc(GL_EQUAL, 0x0, 0xFF);                      // draw only if stencil value is 0
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP); // do not write to the stencil buffer.

        modelview.push(glm::mat4(1.0));
        modelview.top() = modelview.top() * viewMat;

        // send the data for the ith light
        glm::vec4 pos = lights[lightIndex].getPosition();
        pos = lightTransformations[lightIndex] * pos;
        // position
        // adding direction for spotlight
        glm::vec4 spotDirection = lights[lightIndex].getSpotDirection();
        spotDirection = lightTransformations[lightIndex] * spotDirection;
        // Set light colors
        // TODO : Update this code for PBR instead of phong. - Done
        glUniform3fv(renderShaderLocations.getLocation("light.color"), 1, glm::value_ptr(lights[lightIndex].getColor()));
        glUniform4fv(renderShaderLocations.getLocation("light.position"), 1, glm::value_ptr(pos));
        // spotlight stuff here
        glUniform1f(renderShaderLocations.getLocation("light.spotAngleCosine"), cos(glm::radians(lights[lightIndex].getSpotCutoff())));
        glUniform3fv(renderShaderLocations.getLocation("light.spotDirection"), 1, glm::value_ptr(spotDirection));

        // send projection matrix to GPU
        glUniformMatrix4fv(renderShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // draw scene graph here
        scenegraph->getRoot()->accept(renderer);

        modelview.pop();
        renderProgram.disable();
    }

    void PBRSVIBLPipeline::updateProjection(glm::mat4& newProjection)
    {
        projection = glm::mat4(newProjection);

        // need to create different textures when the screen is resized!

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        glDeleteTextures(1, &hdrColorBuffer);
        glDeleteRenderbuffers(1, &depthStencilBuffer);


        glGenTextures(1, &hdrColorBuffer);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, viewport[2], viewport[3], 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenRenderbuffers(1, &depthStencilBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, viewport[2], viewport[3]);
        
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0); // sets color texture as color attachment
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilBuffer);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw runtime_error("Resized framebuffer is not complete!");
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

#endif