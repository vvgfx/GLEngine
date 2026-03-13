#ifndef _TEXTUREDPBRPIPELINE_H_
#define _TEXTUREDPBRPIPELINE_H_

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
#include "../sgraph/TexturedPBRRenderer.h"
#include "../sgraph/LightRetriever.h"
#include "TangentComputer.h"
#include "glad/glad.h"

namespace pipeline
{
    /**
     * An implementation the pipeline interface. This pipeline features lights (directional and spotlights), textures and PBR workflow.
     * Note that this pipeline REQUIRES PBR textures to be defined to work properly.
     * To use this pipeline, initalize it using init() and draw a single frame using drawFrame()
     */
    class TexturedPBRPipeline : public AbstractPipeline
    {

    public:
        inline void init(map<string, util::PolygonMesh<VertexAttrib>>& meshes, glm::mat4 &projection, map<string, unsigned int>& texMap);
        inline void addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh);
        inline void drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat);
        inline void initLights(sgraph::IScenegraph *scenegraph);
        inline void initShaderVars();

    private:
        util::ShaderProgram shaderProgram;
        util::ShaderProgram hdrShaderProgram;
        util::ShaderLocationsVault shaderLocations;
        util::ShaderLocationsVault hdrShaderLocations;
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
        unsigned int hdrFBO, hdrColorBuffer, hdrDepthStencilBuffer;

        map<string, string> shaderVarsToVertexAttribs;
    };

    void TexturedPBRPipeline::init(map<string, util::PolygonMesh<VertexAttrib>>& meshes, glm::mat4 &proj, map<string, unsigned int>& texMap)
    {
        this->projection = proj;
        shaderProgram.createProgram("shaders/PBR/TexturePBR.vert",
                                    "shaders/PBR/TexturePBR.frag");
        shaderProgram.enable();
        shaderLocations = shaderProgram.getAllShaderVariables();
        shaderProgram.disable();


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

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        glGenFramebuffers(1, &hdrFBO);

        glGenTextures(1, &hdrColorBuffer);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, viewport[2], viewport[3], 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // depth render buffer.
        glGenRenderbuffers(1, &hdrDepthStencilBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthStencilBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, viewport[2], viewport[3]); // marks this as a depth render buffer


        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0); // sets color texture as color attachment
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hdrDepthStencilBuffer); // sets depth render buffer as depth attachment


        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthStencilBuffer);
        GLint fmt;
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &fmt);

        if(fmt == GL_DEPTH_COMPONENT32)
            std::cout<<"depth buffer format is correct! \n";

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw runtime_error("custom framebuffer is not complete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        initialized = true;
    }

    void TexturedPBRPipeline::addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh)
    {
        TangentComputer::computeTangents(mesh);
        util::ObjectInstance *obj = new util::ObjectInstance(objectName);
        obj->initPolygonMesh(shaderLocations, shaderVarsToVertexAttribs, mesh);
        objects[objectName] = obj;
    }

    void TexturedPBRPipeline::drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        if (!initialized)
            throw runtime_error("pipeline has not been initialized.");


        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        
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

        scenegraph->getRoot()->accept(renderer);

        if(cubeMapLoaded)
            drawCubeMap(viewMat);

        modelview.pop();
        shaderProgram.disable();

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

        
        glFlush();
    }

    void TexturedPBRPipeline::initLights(sgraph::IScenegraph *scenegraph)
    {
        sgraph::LightRetriever *lightsParser = reinterpret_cast<sgraph::LightRetriever *>(lightRetriever);
        lightsParser->clearData();
        scenegraph->getRoot()->accept(lightRetriever);
        lights = lightsParser->getLights();
        lightTransformations = lightsParser->getLightTransformations();
    }

    void TexturedPBRPipeline::initShaderVars()
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