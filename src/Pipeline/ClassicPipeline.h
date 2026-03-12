#ifndef _CLASSICPIPELINE_H_
#define _CLASSICPIPELINE_H_

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
#include "../sgraph/GLScenegraphRenderer.h"
#include "../sgraph/LightRetriever.h"
#include "TangentComputer.h"

namespace pipeline
{
    /**
     * An implementation the pipeline interface. This pipeline features lights (directional and spotlights) and textures.
     * To use this pipeline, initalize it using init() and draw a single frame using drawFrame()
     */
    class ClassicPipeline : public AbstractPipeline
    {

    public:
        inline void initTextures(map<string, util::TextureImage *> &textureMap);
        inline void init(map<string, util::PolygonMesh<VertexAttrib>>& meshes, map<string, util::TextureImage *>& texMap, glm::mat4 &projection);
        inline void addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh);
        inline void drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat);
        inline void initLights(sgraph::IScenegraph *scenegraph);
        inline void initShaderVars();

    private:
        util::ShaderProgram shaderProgram;
        util::ShaderLocationsVault shaderLocations;
        sgraph::SGNodeVisitor *renderer;
        sgraph::SGNodeVisitor *lightRetriever;
        map<string, unsigned int> textureIdMap;
        vector<util::Light> lights;
        vector<glm::mat4> lightTransformations;
        std::map<string, sgraph::TransformNode *> cachedNodes;
        vector<LightLocation> lightLocations;
        bool initialized = false;
        int frames;
        double time;

        map<string, string> shaderVarsToVertexAttribs;
    };

    void ClassicPipeline::init(map<string, util::PolygonMesh<VertexAttrib>>& meshes, map<string, util::TextureImage *>& texMap, glm::mat4 &proj)
    {
        this->projection = proj;
        shaderProgram.createProgram("shaders/phong-multiple.vert",
                                    "shaders/phong-multiple.frag");
        shaderProgram.enable();
        shaderLocations = shaderProgram.getAllShaderVariables();
        shaderProgram.disable();

        // Mapping of shader variables to vertex attributes
        shaderVarsToVertexAttribs["vPosition"] = "position";
        shaderVarsToVertexAttribs["vNormal"] = "normal";
        shaderVarsToVertexAttribs["vTexCoord"] = "texcoord";
        shaderVarsToVertexAttribs["vTangent"] = "tangent";

        for (typename map<string, util::PolygonMesh<VertexAttrib>>::iterator it = meshes.begin();
             it != meshes.end();
             it++)
        {
            cout << "computing tangents" << endl;
            TangentComputer::computeTangents(it->second); // uncomment later
            util::ObjectInstance *obj = new util::ObjectInstance(it->first);
            obj->initPolygonMesh(shaderLocations, shaderVarsToVertexAttribs, it->second);
            objects[it->first] = obj;
        }
        initTextures(texMap);
        renderer = new sgraph::GLScenegraphRenderer(modelview, objects, shaderLocations, textureIdMap);
        lightRetriever = new sgraph::LightRetriever(modelview);
        initialized = true;
    }

    void ClassicPipeline::addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh)
    {
        TangentComputer::computeTangents(mesh);
        util::ObjectInstance *obj = new util::ObjectInstance(objectName);
        obj->initPolygonMesh(shaderLocations, shaderVarsToVertexAttribs, mesh);
        objects[objectName] = obj;
    }

    void ClassicPipeline::drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4 &viewMat)
    {
        if (!initialized)
            throw runtime_error("pipeline has not been initialized.");

        shaderProgram.enable();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        modelview.push(glm::mat4(1.0f));
        modelview.top() = modelview.top() * viewMat;
        initLights(scenegraph); // lighting scenegraph traversal happens here. I've moved this to the first because the lights need to be initialized
        initShaderVars();
        modelview.pop();

        modelview.push(glm::mat4(1.0));
        modelview.top() = modelview.top() * viewMat;
        glUniform1i(shaderLocations.getLocation("numLights"), lights.size());
        for (int i = 0; i < lights.size(); i++)
        {
            glm::vec4 pos = lights[i].getPosition();
            pos = lightTransformations[i] * pos;
            glm::vec4 spotDirection = lights[i].getSpotDirection();
            spotDirection = lightTransformations[i] * spotDirection;
            // cout<<"ambient: "<<lightLocations[i].ambient<<endl;
            // Set light colors
            glUniform3fv(lightLocations[i].ambient, 1, glm::value_ptr(lights[i].getAmbient()));
            glUniform3fv(lightLocations[i].diffuse, 1, glm::value_ptr(lights[i].getDiffuse()));
            glUniform3fv(lightLocations[i].specular, 1, glm::value_ptr(lights[i].getSpecular()));
            glUniform4fv(lightLocations[i].position, 1, glm::value_ptr(pos));
            // spotlight stuff here
            glUniform1f(lightLocations[i].spotAngle, lights[i].getSpotCutoff());
            glUniform3fv(lightLocations[i].spotDirection, 1, glm::value_ptr(spotDirection));
        }

        glUniformMatrix4fv(shaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

        scenegraph->getRoot()->accept(renderer);
        // cout<<"Errors: "<<glGetError()<<endl;
        modelview.pop();
        glFlush();
        shaderProgram.disable();
    }

    void ClassicPipeline::initTextures(map<string, util::TextureImage *> &textureMap)
    {
        for (typename map<string, util::TextureImage *>::iterator it = textureMap.begin(); it != textureMap.end(); it++)
        {
            // first - name of texture, second - texture itself
            util::TextureImage *textureObject = it->second;

            // generate texture ID
            unsigned int textureId;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);

            // texture params
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Mipmaps are not available for maximization

            // copy texture to GPU
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureObject->getWidth(), textureObject->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, textureObject->getImage());
            glGenerateMipmap(GL_TEXTURE_2D);

            // save id in map
            textureIdMap[it->first] = textureId;
        }
    }

    void ClassicPipeline::initLights(sgraph::IScenegraph *scenegraph)
    {
        sgraph::LightRetriever *lightsParser = reinterpret_cast<sgraph::LightRetriever *>(lightRetriever);
        lightsParser->clearData();
        scenegraph->getRoot()->accept(lightRetriever);
        lights = lightsParser->getLights();
        lightTransformations = lightsParser->getLightTransformations();
    }

    void ClassicPipeline::initShaderVars()
    {
        // cout<<"setting lightlocations"<<endl;
        lightLocations.clear();
        for (int i = 0; i < lights.size(); i++)
        {
            LightLocation ll;
            stringstream name;

            name << "light[" << i << "]";
            ll.ambient = shaderLocations.getLocation(name.str() + "" + ".ambient");
            ll.diffuse = shaderLocations.getLocation(name.str() + ".diffuse");
            ll.specular = shaderLocations.getLocation(name.str() + ".specular");
            ll.position = shaderLocations.getLocation(name.str() + ".position");
            // adding spotDirection and spotAngle.
            ll.spotDirection = shaderLocations.getLocation(name.str() + ".spotDirection");
            ll.spotAngle = shaderLocations.getLocation(name.str() + ".spotAngle");
            lightLocations.push_back(ll);
        }
    }
}

#endif