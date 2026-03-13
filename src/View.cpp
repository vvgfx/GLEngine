#include "View.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace std;
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "sgraph/GLScenegraphRenderer.h"
#include "sgraph/LightRetriever.h"
#include "sgraph/ShadowRenderer.h"
#include "sgraph/DepthRenderer.h"
#include "sgraph/AmbientRenderer.h"
#include "VertexAttrib.h"
#include "Pipeline/ShadowVolumePipeline.h"
#include "Pipeline/ShadowVolumeBumpMappingPipeline.h"
#include "Pipeline/BasicPBRPipeline.h"
#include "Pipeline/TexturedPBRPipeline.h"
#include "Pipeline/PBRShadowVolumePipeline.h"
#include "Pipeline/TexturedPBRSVPipeline.h"
#include "Pipeline/PBRSVIBLPipeline.h"
#include "Pipeline/PBRIBLPipeline.h"
#include "Pipeline/GIPipeline.h"


View::View() {

}

View::~View(){

}

void View::init(Callbacks *callbacks,map<string,util::PolygonMesh<VertexAttrib>>& meshes, map<string, unsigned int>& texIdMap, sgraph::IScenegraph* sgraph)
{
    cout<<"Parent view init"<<endl;
#ifdef __linux__
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_STENCIL_BITS, 8);  // enable stencil buffer
    glfwWindowHint(GLFW_DEPTH_BITS, 24);   // For depth testing
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);  // Double buffering

    window = glfwCreateWindow(1280, 720, "Rendering Engine", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
     glfwSetWindowUserPointer(window, (void *)callbacks);

    //using C++ functions as callbacks to a C-style library
    glfwSetKeyCallback(window, 
    [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window))->onkey(key,scancode,action,mods);
    });

    glfwSetWindowSizeCallback(window, 
    [](GLFWwindow* window, int width,int height)
    {
        reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window))->reshape(width,height);
    });

    glfwSetMouseButtonCallback(window, 
    [](GLFWwindow* window, int button, int action, int mods)
    {
        reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window))->onMouseInput(button, action, mods);
    });

    glfwSetCursorPosCallback(window, 
    [](GLFWwindow* window, double xpos, double ypos)
    {
        reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window))->onCursorMove(xpos, ypos);
    });

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1); // VSync!!!

    // initTextures(texMap);
    this->textureIdMap = &texIdMap;
    int window_width,window_height;
    glfwGetFramebufferSize(window,&window_width,&window_height);
    projection = glm::perspective(glm::radians(60.0f),(float)window_width/window_height,0.1f,10000.0f);

    // need to move this to before the pipeline initialization because its queried in the initialization.
    glViewport(0, 0, window_width,window_height);
    #pragma region Pipeline init
    // pipeline = new pipeline::PBRShadowVolumePipeline();
    // reinterpret_cast<pipeline::PBRShadowVolumePipeline*>(pipeline)->init(meshes, projection);
    pipeline = new pipeline::TexturedPBRSVPipeline();
    reinterpret_cast<pipeline::TexturedPBRSVPipeline*>(pipeline)->init(meshes, projection, texIdMap);
    // pipeline = new pipeline::TexturedPBRPipeline();
    // reinterpret_cast<pipeline::TexturedPBRPipeline*>(pipeline)->init(meshes, projection, texIdMap);
    // pipeline = new pipeline::PBRIBLPipeline();
    // reinterpret_cast<pipeline::PBRIBLPipeline*>(pipeline)->init(meshes, projection, texIdMap);
    // pipeline = new pipeline::PBRSVIBLPipeline();
    // reinterpret_cast<pipeline::PBRSVIBLPipeline*>(pipeline)->init(meshes, projection, texIdMap);
    // pipeline = new pipeline::BasicPBRPipeline();
    // reinterpret_cast<pipeline::BasicPBRPipeline*>(pipeline)->init(meshes, projection);
    // pipeline = new pipeline::GIPipeline();
    // reinterpret_cast<pipeline::GIPipeline*>(pipeline)->init(meshes, projection, texIdMap);
    #pragma endregion

    frames = 0;
    time = glfwGetTime();
    initScenegraphNodes(sgraph);
}

void View::computeTangents(util::PolygonMesh<VertexAttrib>& tmesh)
{
    int i, j;
    vector<glm::vec4> tangents;
    vector<float> data;

    vector<VertexAttrib> vertexData = tmesh.getVertexAttributes();
    vector<unsigned int> primitives = tmesh.getPrimitives();
    int primitiveSize = tmesh.getPrimitiveSize();
    int vert1, vert2, vert3;
    if(primitiveSize == 6)
    {
        //GL_TRIANGLES_ADJACENCY
        vert1 = 0;
        vert2 = 2;
        vert3 = 4;
    }
    else
    {
        //GL_TRIANGLES
        vert1 = 0;
        vert2 = 1;
        vert3 = 2;
    }
    //initialize as 0
    for (i = 0; i < vertexData.size(); i++)
        tangents.push_back(glm::vec4(0.0f, 0.0, 0.0f, 0.0f));

    //go through all the triangles
    for (i = 0; i < primitives.size(); i += primitiveSize)
    {
        // cout<<"i: "<<i<<endl;
        int i0, i1, i2;
        i0 = primitives[i + vert1];
        i1 = primitives[i + vert2];
        i2 = primitives[i + vert3];

        //vertex positions
        data = vertexData[i0].getData("position");
        glm::vec3 v0 = glm::vec3(data[0],data[1],data[2]);

        data = vertexData[i1].getData("position");
        glm::vec3 v1 = glm::vec3(data[0],data[1],data[2]);

        data = vertexData[i2].getData("position");
        glm::vec3 v2 = glm::vec3(data[0],data[1],data[2]);

        // UV coordinates
        data = vertexData[i0].getData("texcoord");
        glm::vec2 uv0 = glm::vec2(data[0],data[1]);

        data = vertexData[i1].getData("texcoord");
        glm::vec2 uv1 = glm::vec2(data[0],data[1]);

        data = vertexData[i2].getData("texcoord");
        glm::vec2 uv2 = glm::vec2(data[0],data[1]);

        // Edges of the triangle : position delta
        glm::vec3 deltaPos1 = v1 - v0;
        glm::vec3 deltaPos2 = v2 - v0;

        // UV delta
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        glm::vec3 tangent = ((deltaPos1 * deltaUV2.y) - (deltaPos2 * deltaUV1.y)) * r;

        // change this to support both triangles and triangles adjacency.
        // This accumulates the tangents for each vertex so that the final vertex tangent is smooth.
        tangents[primitives[i + vert1]] = tangents[primitives[i + vert1]] + glm::vec4(tangent, 0.0f);
        tangents[primitives[i + vert2]] = tangents[primitives[i + vert2]] + glm::vec4(tangent, 0.0f);
        tangents[primitives[i + vert3]] = tangents[primitives[i + vert3]] + glm::vec4(tangent, 0.0f);

        // for (j = 0; j < 3; j++) {
        //     tangents[primitives[i + j]] =
        //         tangents[primitives[i + j]] + glm::vec4(tangent, 0.0f);
        //     }
        // }
    }
        //orthogonalization
    for (i = 0; i < tangents.size(); i++) 
    {
        glm::vec3 t = glm::vec3(tangents[i].x,tangents[i].y,tangents[i].z);
        t = glm::normalize(t);
        data = vertexData[i].getData("normal");
        glm::vec3 n = glm::vec3(data[0],data[1],data[2]);

        glm::vec3 b = glm::cross(n,t);
        t = glm::cross(b,n);

        t = glm::normalize(t);

        tangents[i] = glm::vec4(t,0.0f);
    }

    // set the vertex data
    for (i = 0; i < vertexData.size(); i++)
    {
        data.clear();
        data.push_back(tangents[i].x);
        data.push_back(tangents[i].y);
        data.push_back(tangents[i].z);
        data.push_back(tangents[i].w);

        vertexData[i].setData("tangent", data);
    }
    tmesh.setVertexData(vertexData);
}


void View::Resize()
{
    int window_width,window_height;
    glfwGetFramebufferSize(window,&window_width,&window_height);
    projection = glm::perspective(glm::radians(60.0f),(float)window_width/window_height,0.1f,10000.0f);
    pipeline->updateProjection(projection);
}

void View::updateTrackball(float deltaX, float deltaY)
{
    float sensitivity = 0.005f;
    glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0), (deltaX * sensitivity), glm::vec3(0.0f, 1.0f, 0.0f));
    if(cameraType != 2)
        rotMatrix = glm::rotate(rotMatrix, (deltaY * sensitivity), glm::vec3(1.0f, 0.0f, 0.0f));
    dynamic_cast<sgraph::DynamicTransform*>(cachedNodes["trackball"])->premulTransformMatrix(rotMatrix);
}

void View::resetTrackball()
{
    dynamic_cast<sgraph::DynamicTransform*>(cachedNodes["trackball"])->setTransformMatrix(glm::mat4(1.0f));
}


void View::display(sgraph::IScenegraph *scenegraph) 
{
    
    #pragma region lightSetup
    // setting up the view matrices beforehand because all render calculations are going to be on the view coordinate system.

    glm::mat4 viewMat(1.0f);
    if(cameraType == 1)
        viewMat = viewMat * glm::lookAt(glm::vec3(0.0f, 0.0f, 100.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    else if(cameraType == 2)
        viewMat = viewMat * glm::lookAt(glm::vec3(0.0f, 150.0f, 300.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    else if(cameraType == 3)
    {
            //Drone camera. Need to find a point that is forward(for the lookAt), find the drone co-ordinates(for the eye) and the up-direction for the up-axis
            //drone co-ordinates seem simple enough. I can just use the transform matrix with a translation of 20 in the z-axis
            //target = same as eye, the translation must be higher, so 25?
            //for the up-axis, I can convert the y axis to vec4, pre-multiply by the transformation matrix, then convert back to vec3.
            //This seems super hacky though, is there an alternate way that's easier?
            
            glm::mat4 droneTransformMatrix = dynamic_cast<sgraph::DynamicTransform*>(cachedNodes["drone-movement"])->getTransformMatrix();
            glm::vec3 droneEye = droneTransformMatrix * glm::vec4(0.0f, 0.0f, 20.0f, 1.0f); // setting 1 as the homogenous coordinate
            // Implicit typecasts work!!!!
            glm::vec3 droneLookAt = droneTransformMatrix * glm::vec4(0.0f, 0.0f, 25.0f, 1.0f);
            glm::vec3 droneUp = droneTransformMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);//homogenous coordinate is 0.0f as the vector is an axis, not a point.
            
            viewMat = viewMat * glm::lookAt(droneEye, droneLookAt, droneUp);        
    }
    #pragma endregion


    #pragma region pipeline

    pipeline->drawFrame(scenegraph, viewMat);

    #pragma endregion
    
    
    
    
    glFlush();
    glfwSwapBuffers(window);
    glfwPollEvents();
    frames++;
    double currenttime = glfwGetTime();
    if ((currenttime-time)>1.0) {
        printf("Framerate: %2.0f\r",frames/(currenttime-time));
        frames = 0;
        time = currenttime;
    }
    

}



void View::initLights(sgraph::IScenegraph *scenegraph)
{
    sgraph::LightRetriever* lightsParser = reinterpret_cast<sgraph::LightRetriever*>(lightRetriever);
    lightsParser->clearData();
    scenegraph->getRoot()->accept(lightRetriever);
    lights = lightsParser->getLights();
    lightTransformations = lightsParser->getLightTransformations();
}

void View::initLightShaderVars()
{
    lightLocations.clear();
    for (int i = 0; i < lights.size(); i++)
    {
      LightLocation ll;
      stringstream name;

      name << "light[" << i << "]";
      ll.ambient = renderShaderLocations.getLocation(name.str() + "" +".ambient");
      ll.diffuse = renderShaderLocations.getLocation(name.str() + ".diffuse");
      ll.specular = renderShaderLocations.getLocation(name.str() + ".specular");
      ll.position = renderShaderLocations.getLocation(name.str() + ".position");
      //adding spotDirection and spotAngle.
      ll.spotDirection = renderShaderLocations.getLocation(name.str() + ".spotDirection");
      ll.spotAngle = renderShaderLocations.getLocation(name.str() + ".spotAngle");
      lightLocations.push_back(ll);
    }
}

bool View::shouldWindowClose() {
    return glfwWindowShouldClose(window);
}

void View::switchShaders()
{
    // Not supporting toon shaders for shadow volumes and pbr
    // isToonShaderUsed = !isToonShaderUsed;
    // if(isToonShaderUsed)
    //     program.createProgram(string("shaders/toon.vert"),string("shaders/toon.frag"));
    // else
    //     program.createProgram(string("shaders/phong-multiple.vert"),string("shaders/phong-multiple.frag"));
    // program.enable();
    // renderShaderLocations = program.getAllShaderVariables();
    // cout<<"toon shader status: "<<isToonShaderUsed<<endl;
}



void View::closeWindow() {
    for (map<string,util::ObjectInstance *>::iterator it=objects.begin();
           it!=objects.end();
           it++) {
          it->second->cleanup();
          delete it->second;
    } 
    glfwDestroyWindow(window);

    glfwTerminate();
}

// This saves all the nodes required for dynamic transformation
void View::initScenegraphNodes(sgraph::IScenegraph *scenegraph)
{
    auto nodes = scenegraph->getNodes();
    std::vector<string> savedNodes = {"propeller-1-rotate", "propeller-2-rotate", "propeller-3-rotate", "propeller-4-rotate",
                                         "trackball", "drone-movement", "camera"};

    for(const auto& nodeName: savedNodes)
    {
        if(nodes->find(nodeName) != nodes->end())
        {
            // The node is present, save it!
            cout<<"Found : "<<nodeName<<endl; //It's finding the nodes now.
            cachedNodes[nodeName] = dynamic_cast<sgraph::TransformNode*>((*nodes)[nodeName]); // Can't cast to abstract class, so need to cast to pointer 
        }
    }
}


void View::rotatePropeller(string nodeName, float time)
{
    float rotationSpeed = speed * 200.0f;

    float rotationAngle = glm::radians(rotationSpeed * time);

    sgraph::RotateTransform *propellerNode = dynamic_cast<sgraph::RotateTransform*>(cachedNodes[nodeName]);
    if(propellerNode)
    {
        propellerNode->updateRotation(rotationAngle);
    }
}


void View::changePropellerSpeed(int num)
{
    //positive number = increments speed; negative number = decrements speed

    float speedSensitivity = 0.25f;
    speed += num > 0 ? speedSensitivity : -speedSensitivity;

    speed = speed <= 0 ? 0.1f : speed >= 3 ? 3.0f : speed;
}

void View::startRotation()
{

    cout<<"starting rotation!"<<endl;
    isRotating = true;
}

void View::rotate()
{
    if(!isRotating)
        return;
    
    if(rotationAngle > 360.0f)
    {
        rotationAngle = 0.0f;
        isRotating = false;
    }
    float rollSpeed = 2.0f;
    rotationAngle += rollSpeed;

    float newAngle = glm::radians(rotationAngle);
    sgraph::DynamicTransform *droneRotateNode = dynamic_cast<sgraph::DynamicTransform*>(cachedNodes["drone-movement"]);
    droneRotateNode->postmulTransformMatrix(glm::rotate(glm::mat4(1.0), glm::radians(rollSpeed), glm::vec3(0.0f, 0.0f, 1.0f)));
    
}

/**
 * Move/Rotate the drone by passing the matrix to postmultiply. This stacks on top of previous input.
 */
void View::moveDrone(int direction)
{
    glm::mat4 translateMatrix(1.0);
    float directionalSpeed = (direction > 0 ? 1.0f : -1.0f) * speed * 5.0f;
    translateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, directionalSpeed));
    sgraph::DynamicTransform *droneTranslateNode = dynamic_cast<sgraph::DynamicTransform*>(cachedNodes["drone-movement"]);
    droneTranslateNode->postmulTransformMatrix(translateMatrix);
}


/**
 * Set the drone's matrix to what is passed. A good idea would be to pass a rotation and a translation.
 */
void View::setDroneOrientation(glm::mat4 resetMatrix)
{
    sgraph::DynamicTransform* droneNode = dynamic_cast<sgraph::DynamicTransform*>(cachedNodes["drone-movement"]);
    if(droneNode)
        droneNode->setTransformMatrix(resetMatrix);
}

/**
 * Pass a positive yawDir to rotate left, negative to rotate right. Similarly, positive pitchDir to rotate upwards, negative to rotate downwards
 */

void View::rotateDrone(int yawDir, int pitchDir)
{
    //Check if yaw Rotation is present
    glm::mat4 rotationMatrix(1.0f);
    if(yawDir != 0)
    {
        float yawSpeed = (yawDir > 0.0f? 1.0f : -1.0f) * 5.0f;
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(yawSpeed) , glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if(pitchDir != 0)
    {
        float pitchSpeed = (pitchDir > 0.0f? 1.0f : -1.0f) * 5.0f;
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(pitchSpeed) , glm::vec3(1.0f, 0.0f, 0.0f));
    }

    dynamic_cast<sgraph::DynamicTransform*>(cachedNodes["drone-movement"])->postmulTransformMatrix(rotationMatrix);
}

void View::changeCameraType(int cameraType)
{
    this->cameraType = cameraType;
}