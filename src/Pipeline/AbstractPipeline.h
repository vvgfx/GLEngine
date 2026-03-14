#ifndef _ABSTRACT_PIPELINE_H_
#define _ABSTRACT_PIPELINE_H_

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#include <map>
#include <iostream>
#include <ObjectInstance.h>
#include <ShaderProgram.h>
#include <ShaderLocationsVault.h>
#include "../sgraph/IScenegraph.h"
#include "IPipeline.h"
using namespace std;


namespace pipeline
{
    /**
     * This class holds common functions in a render pipeline.
     */

    class AbstractPipeline : public IPipeline
    {
        public:
        virtual ~AbstractPipeline() {}

        virtual void updateProjection(glm::mat4& newProjection)
        {
            projection = glm::mat4(newProjection);
        }

        virtual void drawCubeMap(glm::mat4 &viewMat)
        {
            glDepthMask(GL_FALSE);
            cubeMapProgram.enable();
            modelview.push(glm::mat4(glm::mat3(viewMat))); // doing this to remove translations by extracting the upper-left 3x3 matrix.
            glUniformMatrix4fv(cubeMapShaderLocations.getLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(cubeMapShaderLocations.getLocation("modelview"), 1, GL_FALSE, glm::value_ptr(modelview.top()));

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureId);
            glUniform1i(cubeMapShaderLocations.getLocation("skybox"), 1);

            glDepthFunc(GL_LEQUAL);
            objects["skybox"]->draw();

            modelview.pop();
            cubeMapProgram.disable();

            glDepthMask(GL_TRUE);
        }

        virtual void loadCubeMap(vector<util::TextureImage*>& cubeMap)
        {
            if(cubeMap.size() ==  0 || cubeMap.size() == 1) // default implementation does not support HDR images.
                return;
            if(cubeMap.size() !=  6)
                throw runtime_error("cubemap size is not 6! likely missing an overriddden implementation");
            
            glGenTextures(1, &cubemapTextureId);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureId);
            int i = 0;
            for(util::TextureImage* texImage : cubeMap)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, texImage->getWidth(), texImage->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, texImage->getImage());
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                i++;
            }
            
            cubeMapProgram.createProgram("shaders/cubemap/cubemap.vert", "shaders/cubemap/cubemapHDR.frag");
            cubeMapProgram.enable();
            cubeMapShaderLocations = cubeMapProgram.getAllShaderVariables();
            cubeMapProgram.disable();
            cubeMapLoaded = true;
        }

        virtual void keyCallback(int key)
        {
            // do nothing, this should be overridden in the lower classes.
        }
        
        protected:
        glm::mat4 projection;
        stack<glm::mat4> modelview;
        glm::vec3 cameraPos;
        map<string, util::ObjectInstance *> objects;


        unsigned int cubemapTextureId;
        bool cubeMapLoaded = false;

        util::ShaderProgram cubeMapProgram;
        util::ShaderLocationsVault cubeMapShaderLocations;
    };

}

#endif