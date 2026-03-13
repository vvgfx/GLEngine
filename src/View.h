#ifndef __VIEW_H__
#define __VIEW_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <ShaderGeoProgram.h> // This is for silhouettes/shadow volumes/anything that requires geometry shaders.
#include <ShaderProgram.h> // This is for normal rendering
#include <ComputeProgram.h> // For compute shaders
#include "sgraph/SGNodeVisitor.h"
#include "ObjectInstance.h"
#include "PolygonMesh.h"
#include "VertexAttrib.h"
#include "Callbacks.h"
#include "sgraph/IScenegraph.h"
#include "TextureImage.h"
#include <stack>
#include "Pipeline/ClassicPipeline.h"
using namespace std;


class View
{
public:
    View();
    ~View();
    virtual void init(Callbacks* callbacks,map<string,util::PolygonMesh<VertexAttrib>>& meshes, map<string, unsigned int>& texIdMap, sgraph::IScenegraph* sgraph);
    virtual void Resize();
    virtual void display(sgraph::IScenegraph *scenegraph);
    virtual bool shouldWindowClose();
    virtual void closeWindow();
    virtual void updateTrackball(float deltaX, float deltaY);
    virtual void resetTrackball();
    virtual void initScenegraphNodes(sgraph::IScenegraph *scenegraph);
    virtual void changePropellerSpeed(int num);
    virtual void rotatePropeller(string nodename, float time);
    virtual void startRotation();
    virtual void moveDrone(int direction);
    virtual void setDroneOrientation(glm::mat4 resetMatrix);
    virtual void rotateDrone(int yawDir, int pitchDir);
    virtual void changeCameraType(int type);
    virtual void initLights(sgraph::IScenegraph *scenegraph);
    virtual void switchShaders();
    float xDelta, yDelta, zDelta;
    
    //This class saves the shader locations of all the light inputs.
    class LightLocation 
    {
        
        public:
        int ambient,diffuse,specular,position;
        int spotDirection, spotAngle;
        LightLocation()
        {
            ambient = diffuse = specular = position = -1;
            spotDirection = spotAngle = -1;
        }
        
    };
    
    protected: 
    virtual void initLightShaderVars();
    virtual void rotate();
    virtual void computeTangents(util::PolygonMesh<VertexAttrib>& mesh);
    GLFWwindow* window;
    pipeline::IPipeline* pipeline;
    util::ShaderProgram renderProgram;
    util::ShaderGeoProgram shadowProgram;
    util::ShaderProgram depthProgram;
    util::ShaderProgram ambientProgram;
    util::ShaderLocationsVault renderShaderLocations;
    util::ShaderLocationsVault shadhowShaderLocations;
    util::ShaderLocationsVault depthShaderLocations;
    util::ShaderLocationsVault ambientShaderLocations;
    map<string,util::ObjectInstance *> objects;
    glm::mat4 projection;
    stack<glm::mat4> modelview;
    sgraph::SGNodeVisitor *renderer;
    sgraph::SGNodeVisitor *lightRetriever;
    sgraph::SGNodeVisitor *shadowRenderer;
    sgraph::SGNodeVisitor *depthRenderer;
    sgraph::SGNodeVisitor *ambientRenderer;
    int frames;
    double time;
    float rotationSpeed = 0.5f;
    float speed = 1.0f;

    bool isRotating = false;
    float rotationAngle = 0.0f;

    int cameraType = 1;

    //Saving all the required nodes for dynamic transformation!
    std::map<string, sgraph::TransformNode*> cachedNodes; // Need to save this as a pointer because TransformNode is abstract :(
    vector<LightLocation> lightLocations;
    vector<util::Light> lights;
    vector<glm::mat4> lightTransformations;
    bool isToonShaderUsed = false;
    map<string, unsigned int>* textureIdMap;
};

#endif