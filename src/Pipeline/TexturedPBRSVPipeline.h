#ifndef _TEXTUREDPBRSVPIPELINE_H_
#define _TEXTUREDPBRSVPIPELINE_H_

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
#include "../sgraph/TexturedPBRAmbientRenderer.h"
#include "TangentComputer.h"
#include <iostream>

#include "../sgraph/STBImageLoader.h"

namespace pipeline
{
    /**
     * An implementation the pipeline interface. This pipeline features lights (directional and spotlights), shadows using shadow volumes and PBR workflow.
     * Note that this pipeline REQUIRES PBR materials to be defined to work properly, and supports (and requires) textures.
     * To use this pipeline, initalize it using init() and draw a single frame using drawFrame()
     */
    class TexturedPBRSVPipeline : public AbstractPipeline
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
        //testing cubemaps
    private:
        
        util::ShaderProgram renderProgram;
        util::ShaderProgram depthProgram;
        util::ShaderProgram ambientProgram;
        util::ShaderProgram hdrShaderProgram;
        util::ShaderGeoProgram shadowProgram;

        
        util::ShaderLocationsVault renderShaderLocations;
        util::ShaderLocationsVault depthShaderLocations;
        util::ShaderLocationsVault ambientShaderLocations;
        util::ShaderLocationsVault shadowShaderLocations;
        util::ShaderLocationsVault hdrShaderLocations;

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
        unsigned int hdrFBO, hdrColorBuffer, hdrDepthStencilBuffer;

        unsigned int quadVAO = 0;
        unsigned int quadVBO;


    };

    void TexturedPBRSVPipeline::init(map<string, util::PolygonMesh<VertexAttrib>> &meshes, glm::mat4 &proj, map<string, unsigned int>& texMap)
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
        ambientProgram.createProgram("shaders/shadow/Texture-PBR-ambient.vert",
                                     "shaders/shadow/Texture-PBR-ambient.frag");
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
        ambientRenderer = new sgraph::TexturedPBRAmbientRenderer(modelview, objects, ambientShaderLocations, *textureIdMap);
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
        glGenRenderbuffers(1, &hdrDepthStencilBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthStencilBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, viewport[2], viewport[3]);

        // attach to custom framebuffer now.
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0); // sets color texture as color attachment
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdrDepthStencilBuffer);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw runtime_error("custom framebuffer is not complete!");
        
        // finally set the default framebuffer back
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    void TexturedPBRSVPipeline::addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh)
    {
        TangentComputer::computeTangents(mesh);
        util::ObjectInstance *obj = new util::ObjectInstance(objectName);
        obj->initPolygonMesh(renderShaderLocations, shaderVarsToVertexAttribs, mesh);
        objects[objectName] = obj;
    }

    void TexturedPBRSVPipeline::drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        if (!initialized)
            throw runtime_error("pipeline has not been initialized.");

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
        // Note to self: In order to do postprocessing, I might need to write the output to a different framebuffer and then read that as a texture to my post-processing pass
        // cout<<"Errors :"<<glGetError()<<endl;

        //test cubemap draw
        // cubeMapDraw(viewMat);
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
        glDisable(GL_DEPTH_TEST); // not sure why this is needed?
        // draw screen space quad
        objects["postProcess"]->draw();
        hdrShaderProgram.disable();

    }

    void TexturedPBRSVPipeline::initLights(sgraph::IScenegraph *scenegraph)
    {
        sgraph::LightRetriever *lightsParser = reinterpret_cast<sgraph::LightRetriever *>(lightRetriever);
        lightsParser->clearData();
        scenegraph->getRoot()->accept(lightRetriever);
        lights = lightsParser->getLights();
        lightTransformations = lightsParser->getLightTransformations();
    }

    void TexturedPBRSVPipeline::depthPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
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
    }

    void TexturedPBRSVPipeline::shadowStencilPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat, int lightIndex)
    {
        glDepthMask(GL_FALSE);             // do not write into the depth buffer anymore. This is so that the shadow volumes do not obstruct the actual objects.
        glEnable(GL_DEPTH_CLAMP);          // Don't want to clip the back polygons.
        glDisable(GL_CULL_FACE);           // Don't want the back-facing polygons to get culled. need them to increment the stencil buffer.
        glStencilFunc(GL_ALWAYS, 0, 0xff); // Always pass the stencil test, reference value 0,mask value 1(should probably use ~0)

        // For some reason, the depth-fail method breaks down for some fragments of the sphere.
        // depth fail
        // glStencilOpSeparate(GL_BACK, // for backfacing polygons
        //                     GL_KEEP, // stencil test fails - doesnt happen because stencil function is set to always pass
        //                     GL_INCR_WRAP, // stencil passes but depth fails - our required condition - increment the stencil buffer value
        //                     GL_KEEP); // both stencil and depth passes - not relevant; do nothing.

        // glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP); // similarly, for front facing polygons, decrement the stencil buffer

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
    }

    void TexturedPBRSVPipeline::ambientPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        // TODO: Pass the material's albedo and ao textures in PBRAmbientRenderer.
        ambientProgram.enable();
        glEnable(GL_BLEND);           // blend the ambient light.
        glBlendEquation(GL_FUNC_ADD); // blend by addition.
        glBlendFunc(GL_ONE, GL_ONE);  // equal parts of existing and ambient. This is fine because the ambient shader has intensity reduced to 0.2 times.
        modelview.push(glm::mat4(1.0));
        modelview.top() = modelview.top() * viewMat;
        glUniformMatrix4fv(ambientShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
        // This uses only the material's ambient. Ideally it should use each light's ambient as well. Maybe later tho.
        scenegraph->getRoot()->accept(ambientRenderer);
        modelview.pop();
        ambientProgram.disable();
        glDisable(GL_BLEND);
    }

    void TexturedPBRSVPipeline::renderObjectPass(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat, int lightIndex)
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

    void TexturedPBRSVPipeline::updateProjection(glm::mat4& newProjection)
    {
        projection = glm::mat4(newProjection);

        // need to create different textures when the screen is resized!

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        glDeleteTextures(1, &hdrColorBuffer);
        glDeleteRenderbuffers(1, &hdrDepthStencilBuffer);


        glGenTextures(1, &hdrColorBuffer);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, viewport[2], viewport[3], 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenRenderbuffers(1, &hdrDepthStencilBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthStencilBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, viewport[2], viewport[3]);
        
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0); // sets color texture as color attachment
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdrDepthStencilBuffer);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw runtime_error("Resized framebuffer is not complete!");
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

#endif