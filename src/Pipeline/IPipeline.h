#ifndef _IPIPELINE_H_
#define _IPIPELINE_H_

#include <glm/glm.hpp>
#include <TextureImage.h>
#include "../sgraph/IScenegraph.h"
using namespace std;


namespace pipeline
{
    /**
     * This class represents a render pipeline. Each pipeline will implement certain rendering features
     * The pupose of this abstraction is to provide a general framework for building new render pipelines
     * that may or may not choose to implement certain features.
     * 
     * At any point of time, only one pipeline should be active.
     */

    class IPipeline
    {
        public:
        virtual ~IPipeline() {}

        /**
         * This will be called once per frame, passing a scenegraph and a view matrix(world to view tranformation matrix)
         */
        virtual void drawFrame(sgraph::IScenegraph *scenegraph, glm::mat4& viewMat)=0;

        /**
         * Initialize the shader programs and other initializations here.
         */
        // virtual void init()=0; // Maybe this is too specific and doesnt need to be in the base class?

        /**
         * Update the projection matrix. This is called when the window is resized.
         */
        virtual void updateProjection(glm::mat4& newProjection)=0;

        // add mesh here, then implement in all the pipelines.

        virtual void addMesh(string objectName, util::PolygonMesh<VertexAttrib>& mesh)=0;

        virtual void loadCubeMap(vector<util::TextureImage*>& cubeMap)=0;
        virtual void drawCubeMap(glm::mat4& viewMat)=0;

        virtual void keyCallback(int key)=0;

    };

}

#endif