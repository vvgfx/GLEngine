#ifndef _GIPIPELINE_H_
#define _GIPIPELINE_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "AbstractPipeline.h"
#include <GLFW/glfw3.h>
#include "../sgraph/IScenegraph.h"
#include <ShaderProgram.h>
#include <ShaderGeoProgram.h>
#include <ComputeProgram.h>
#include <ShaderLocationsVault.h>
#include "../sgraph/SGNodeVisitor.h"
#include <ObjectInstance.h>
#include <Light.h>
#include "LightLocation.h"
#include <VertexAttrib.h>
#include <TextureImage.h>
#include "../sgraph/TexturedPBRRenderer.h"
#include "../sgraph/VoxelRenderer.h"
#include "../sgraph/DepthRenderer.h"
#include "../sgraph/VoxelDebugRenderer.h"
#include "../sgraph/LightRetriever.h"
#include "TangentComputer.h"
#include "glad/glad.h"

namespace pipeline
{
    /**
     * An implementation the pipeline interface. This pipeline features Global Illumination, lights (directional and spotlights), textures and PBR workflow.
     * Note that this pipeline REQUIRES PBR textures to be defined to work properly.
     * To use this pipeline, initalize it using init() and draw a single frame using drawFrame()
     */
    class GIPipeline : public AbstractPipeline
    {

    public:
        inline void init(map<string, util::PolygonMesh<VertexAttrib>>& meshes, glm::mat4 &projection, map<string, unsigned int>& texMap);
        inline void addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh);
        inline void drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat);
        inline void initLights(sgraph::IScenegraph *scenegraph);
        inline void initShaderVars();
        inline void sendLightDetails(bool voxel);
        inline void clearVoxelImage();
        inline void createMipmap();
        inline void depthPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat);
        inline void keyCallback(int key);

    private:
        util::ShaderProgram shaderProgram;
        util::ShaderGeoProgram voxelProgram;
        util::ShaderProgram voxelDebugProgram;
        util::ComputeProgram mipmapProgram;
        util::ShaderProgram depthProgram;
        util::ShaderLocationsVault shaderLocations;
        util::ShaderLocationsVault voxelShaderLocations;
        util::ShaderLocationsVault voxelDebugShaderLocations;
        util::ShaderLocationsVault mipmapShaderLocations;
        util::ShaderLocationsVault depthShaderLocations;
        sgraph::SGNodeVisitor *renderer;
        sgraph::SGNodeVisitor *voxelRenderer;
        sgraph::SGNodeVisitor *voxelDebugRenderer;
        sgraph::SGNodeVisitor *lightRetriever;
        sgraph::SGNodeVisitor *depthRenderer;
        map<string, unsigned int>* textureIdMap;
        vector<util::Light> lights;
        vector<glm::mat4> lightTransformations;
        std::map<string, sgraph::TransformNode *> cachedNodes;
        vector<LightLocation> voxelLightLocations;
        vector<LightLocation> lightLocations;
        bool initialized = false, nvidiaGPU = false;
        int frames, voxelResolution;
        double time;
        unsigned int voxelImage, voxelFBO;
        map<string, string> shaderVarsToVertexAttribs;

        int giStatus = 1;


        bool debugVoxels = false;
    };

    void GIPipeline::init(map<string, util::PolygonMesh<VertexAttrib>>& meshes, glm::mat4 &proj, map<string, unsigned int>& texMap)
    {
        this->projection = proj;
        voxelProgram.createProgram("shaders/VXGI/voxelize/voxelize.vert",
                                    "shaders/VXGI/voxelize/voxelize.frag",
                                    "shaders/VXGI/voxelize/voxelize.geom");
        voxelProgram.enable();
        voxelShaderLocations = voxelProgram.getAllShaderVariables();
        voxelProgram.disable();

        voxelDebugProgram.createProgram("shaders/VXGI/voxelize/debug.vert",
                                        "shaders/VXGI/voxelize/debug.frag");
        voxelDebugProgram.enable();
        voxelDebugShaderLocations = voxelDebugProgram.getAllShaderVariables();
        voxelDebugProgram.disable();

        // shaderProgram.createProgram("shaders/PBR/TexturePBR.vert",
        //                             "shaders/PBR/TexturePBR.frag");
        shaderProgram.createProgram("shaders/VXGI/Render/GIPBR.vert",
                                    "shaders/VXGI/Render/GIPBR.frag");
        shaderProgram.enable();
        shaderLocations = shaderProgram.getAllShaderVariables();
        shaderProgram.disable();

        mipmapProgram.createProgram("shaders/VXGI/mipmap/computeMipmap.comp");

        mipmapProgram.enable();
        mipmapShaderLocations = mipmapProgram.getAllShaderVariables();
        mipmapProgram.disable();

        depthProgram.createProgram(string("shaders/shadow/depth.vert"),
                                   string("shaders/shadow/depth.frag"));
        depthProgram.enable();
        depthShaderLocations = depthProgram.getAllShaderVariables();
        depthProgram.disable();

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
        voxelRenderer = new sgraph::VoxelRenderer(modelview, objects, voxelShaderLocations, *textureIdMap);
        voxelDebugRenderer = new sgraph::VoxelDebugRenderer(modelview, objects, voxelDebugShaderLocations, *textureIdMap);
        depthRenderer = new sgraph::DepthRenderer(modelview, objects, depthShaderLocations);
        lightRetriever = new sgraph::LightRetriever(modelview);

        // allocate memory for voxelization 3d image.

        voxelResolution = 256;
        glGenTextures(1, &voxelImage);
        glBindTexture(GL_TEXTURE_3D, voxelImage);
        // glTexImage3D(GL_TEXTURE_3D,  0, GL_RGBA16F, voxelResolution, voxelResolution, voxelResolution, 0, GL_RGBA, GL_HALF_FLOAT, nullptr); // allocates empty memory

        int mipLevels = 1 + floor(log2(voxelResolution)); // no. of mips required to reach 1x1x1
        glTexStorage3D(GL_TEXTURE_3D, mipLevels, GL_RGBA16F, voxelResolution, voxelResolution, voxelResolution);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // use this when generating mipmaps
        // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


        // empty framebuffer for voxelization
        glGenFramebuffers(1, &voxelFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, voxelFBO);
        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, voxelResolution);
        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, voxelResolution);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        initialized = true;
    }

    void GIPipeline::addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh)
    {
        TangentComputer::computeTangents(mesh);
        util::ObjectInstance *obj = new util::ObjectInstance(objectName);
        obj->initPolygonMesh(shaderLocations, shaderVarsToVertexAttribs, mesh);
        objects[objectName] = obj;
    }

    void GIPipeline::drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        if (!initialized)
            throw runtime_error("pipeline has not been initialized.");


        // voxelize the grid and store in 3d float image
        // hardcoding these values for now.
        glm::vec4 gridMin(-50.0f, -50.0f, -50.0f, 1.0f);
        glm::vec4 gridMax(50.0f, 50.0f, 50.0f, 1.0f);

        modelview.push(glm::mat4(1.0f)); // world coordinate space.
        initLights(scenegraph);
        initShaderVars();
        modelview.pop();

        // save the viewport dimensions to revert.
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        // voxel pass
        {
            clearVoxelImage();
            glBindFramebuffer(GL_FRAMEBUFFER, voxelFBO);
            glViewport(0, 0, voxelResolution, voxelResolution);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            
            voxelProgram.enable();
            sendLightDetails(true);
            glBindImageTexture(0, voxelImage, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F); // cannot use shaderLocations for binding!
            glUniform4fv(voxelShaderLocations.getLocation("gridMin"), 1, glm::value_ptr(gridMin));
            glUniform4fv(voxelShaderLocations.getLocation("gridMax"), 1, glm::value_ptr(gridMax));
            scenegraph->getRoot()->accept(voxelRenderer);
            voxelProgram.disable();

            // trying this out to see if I can force the incoherent memory write to work.
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | 
                   GL_TEXTURE_FETCH_BARRIER_BIT);
            
            // mipmap
            glBindTexture(GL_TEXTURE_3D, voxelImage);
            // glGenerateMipmap(GL_TEXTURE_3D); // need to figure out a way to do this without tanking performance!
            createMipmap();
        }

        // restore state
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // can't get the view vec3 directly so doing this.
        glm::mat4 inverseView = glm::inverse(viewMat);
        cameraPos = glm::vec3(inverseView[3]);

        // use 3d image to lookup in rendering pass for indirect lighting.
        if(debugVoxels)
        {
            depthPass(scenegraph, viewMat);
            voxelDebugProgram.enable();
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-0.1f, -0.1f);
            glUniformMatrix4fv(voxelDebugShaderLocations.getLocation("view"), 1, GL_FALSE, glm::value_ptr(viewMat)); // view transformation
            glUniformMatrix4fv(voxelDebugShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, voxelImage);
            glUniform1i(voxelDebugShaderLocations.getLocation("voxelTexture"),  0);
            glUniform4fv(voxelDebugShaderLocations.getLocation("gridMin"), 1, glm::value_ptr(gridMin));
            glUniform4fv(voxelDebugShaderLocations.getLocation("gridMax"), 1, glm::value_ptr(gridMax));

            scenegraph->getRoot()->accept(voxelDebugRenderer);
            glDepthMask(GL_TRUE);
            glDisable(GL_POLYGON_OFFSET_FILL);
            voxelDebugProgram.disable();
            
        }
        else
        {

            // depth pass first.
            depthPass(scenegraph, viewMat);

            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-0.5f, -0.5f);
            shaderProgram.enable();
            // passing the camera location to the fragment shader.
            glUniform3fv(shaderLocations.getLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
            glUniformMatrix4fv(shaderLocations.getLocation("view"), 1, GL_FALSE, glm::value_ptr(viewMat)); // view transformation
            glUniformMatrix4fv(shaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

            // send the voxel 3d image
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_3D, voxelImage);
            glUniform1i(shaderLocations.getLocation("voxelTexture"), 10);
            glUniform4fv(shaderLocations.getLocation("gridMin"), 1, glm::value_ptr(gridMin));
            glUniform4fv(shaderLocations.getLocation("gridMax"), 1, glm::value_ptr(gridMax));
            glUniform1f(shaderLocations.getLocation("voxelResolution"), voxelResolution);
            glUniform1i(shaderLocations.getLocation("useGI"), giStatus);
            sendLightDetails(false);

            scenegraph->getRoot()->accept(renderer);

            glDepthMask(GL_TRUE);
            glDisable(GL_POLYGON_OFFSET_FILL);

            if(cubeMapLoaded)
                drawCubeMap(viewMat);
            modelview.pop();
            glFlush();
            shaderProgram.disable();
        }
    }

    void GIPipeline::depthPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
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
        glDrawBuffer(GL_BACK); // reset state.
    }


    void GIPipeline::clearVoxelImage() 
    {
        static const GLfloat clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        glClearTexImage(voxelImage, 0, GL_RGBA, GL_FLOAT, clearColor);
    }

    void GIPipeline::createMipmap()
    {
        mipmapProgram.enable();
        int mipLevels = 1 + floor(log2(voxelResolution));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, voxelImage);
        glUniform1i(mipmapShaderLocations.getLocation("samplerDownsample"), 0);

        for(int i = 1; i < mipLevels; i++)
        {
            glUniform1i(mipmapShaderLocations.getLocation("Lod"), i-1);

            int mipWidth  = max(1, voxelResolution >> i);
            int mipHeight = max(1, voxelResolution >> i);
            int mipDepth  = max(1, voxelResolution >> i);

            glBindImageTexture(0, voxelImage, i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);// mipmapShaderLocations.getLocation("ImgResult")

            // ceil division
            int groupsX = (mipWidth  + 3) / 4;
            int groupsY = (mipHeight + 3) / 4;
            int groupsZ = (mipDepth  + 3) / 4;
            glDispatchCompute(groupsX, groupsY, groupsZ);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | 
                       GL_TEXTURE_FETCH_BARRIER_BIT);
        }


        mipmapProgram.disable();
    }

    void GIPipeline::initLights(sgraph::IScenegraph *scenegraph)
    {
        sgraph::LightRetriever *lightsParser = reinterpret_cast<sgraph::LightRetriever *>(lightRetriever);
        lightsParser->clearData();
        scenegraph->getRoot()->accept(lightRetriever);
        lights = lightsParser->getLights();
        lightTransformations = lightsParser->getLightTransformations();
    }

    void GIPipeline::sendLightDetails(bool voxel)
    {
        vector<LightLocation>& locationsToUse = voxel ? voxelLightLocations : lightLocations;
        modelview.push(glm::mat4(1.0));
        if(voxel)
            glUniform1i(voxelShaderLocations.getLocation("numLights"), lights.size());
        else
            glUniform1i(shaderLocations.getLocation("numLights"), lights.size());
        
        for (int i = 0; i < lights.size(); i++)
        {
            glm::vec4 pos = lights[i].getPosition();
            pos = lightTransformations[i] * pos; // world coordinate system.
            glm::vec4 spotDirection = lights[i].getSpotDirection();
            spotDirection = lightTransformations[i] * spotDirection;
            // Set light colors
            glUniform3fv(locationsToUse[i].color, 1, glm::value_ptr(lights[i].getColor()));
            glUniform4fv(locationsToUse[i].position, 1, glm::value_ptr(pos));
            // spotlight stuff here
            glUniform1f(locationsToUse[i].spotAngle, cos(glm::radians(lights[i].getSpotCutoff())));
            glUniform3fv(locationsToUse[i].spotDirection, 1, glm::value_ptr(spotDirection));
        }
    }

    void GIPipeline::initShaderVars()
    {
        lightLocations.clear();
        voxelLightLocations.clear();
        for (int i = 0; i < lights.size(); i++)
        {
            stringstream name;
            
            LightLocation ll;
            name << "light[" << i << "]";
            ll.position = shaderLocations.getLocation(name.str() + "" + ".position");
            ll.color = shaderLocations.getLocation(name.str() + ".color");
            ll.spotDirection = shaderLocations.getLocation(name.str() + ".spotDirection");
            ll.spotAngle = shaderLocations.getLocation(name.str() + ".spotAngleCosine");
            lightLocations.push_back(ll);

            LightLocation vll;
            vll.position = voxelShaderLocations.getLocation(name.str() + "" + ".position");
            vll.color = voxelShaderLocations.getLocation(name.str() + ".color");
            vll.spotDirection = voxelShaderLocations.getLocation(name.str() + ".spotDirection");
            vll.spotAngle = voxelShaderLocations.getLocation(name.str() + ".spotAngleCosine");
            voxelLightLocations.push_back(vll);
        }
    }

    void GIPipeline::keyCallback(int key)
    {
        if(key == GLFW_KEY_1)
        {
            giStatus *= -1; 
            cout<<"key 1 received in pipeline, giStatus : "<<giStatus<<endl;
        }
        else if(key == GLFW_KEY_3)
        {
            debugVoxels = !debugVoxels;
        }
    }
}

#endif