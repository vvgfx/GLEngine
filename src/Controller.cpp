#include "Controller.h"
#include "sgraph/ImageLoader.h"
#include "sgraph/PPMImageLoader.h"
#include "sgraph/IScenegraph.h"
#include "sgraph/Scenegraph.h"
#include "sgraph/GroupNode.h"
#include "sgraph/LeafNode.h"
#include "sgraph/ScaleTransform.h"
#include "ObjImporter.h"
using namespace sgraph;
#include <iostream>
using namespace std;

#include "sgraph/ScenegraphExporter.h"
#include "sgraph/ScenegraphImporter.h"
#include "sgraph/ScenegraphDrawer.h"

Controller::Controller(Model* m,View* v, string textfile) :
    mousePressed(false) {
    model = m;
    view = v;
    this->textfile = textfile;
    // initScenegraph();
}

void Controller::initScenegraph() {

     
    
    //read in the file of commands
    ifstream inFile;
    if(textfile == "")
        inFile = ifstream("scenegraphmodels/textured-pbr/texture-pbr-shadow-volume-test.txt");
    else
        inFile = ifstream(textfile);
    sgraph::ScenegraphImporter importer;
    

    IScenegraph *scenegraph = importer.parse(inFile);
    //scenegraph->setMeshes(meshes);
    model->setScenegraph(scenegraph);
    map<string, util::TextureImage*> textureMap = importer.getTextureMap();
    model->saveTextureMap(textureMap);


    cout <<"Scenegraph made" << endl;   
    sgraph::ScenegraphDrawer* drawer = new sgraph::ScenegraphDrawer();
    scenegraph->getRoot()->accept(drawer);
}

Controller::~Controller()
{
    
}

void Controller::run()
{
    cout<<"parent controller run"<<endl;
    sgraph::IScenegraph * scenegraph = model->getScenegraph();
    map<string,util::PolygonMesh<VertexAttrib> > meshes = scenegraph->getMeshes();
    map<string, unsigned int>& texIdMap = model->getTextureIdMap();
    view->init(this,meshes, texIdMap, scenegraph);
    // creating the texture Id maps AFTER init. This is because the OpenGL initialization needs to occur before the textures can be loaded
    model->initTextures(model->getTextureMap());

    //Save the nodes required for transformation when running!
    // view->initScenegraphNodes(scenegraph);
    //Set the initial orientation of the drone!
    glm::mat4 droneOrientation  = glm::translate(glm::mat4(1.0f), glm::vec3(-100.0f, 100.0f, 150.0f));
    view->setDroneOrientation(droneOrientation);
    while (!view->shouldWindowClose()) {
        view->display(scenegraph);
    }
    view->closeWindow();
    exit(EXIT_SUCCESS);
}

void Controller::onkey(int key, int scancode, int action, int mods)
{
    cout << (char)key << " pressed" << endl;

    if(action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    switch(key)
    {
        case GLFW_KEY_R://Rest the trackball orientation
            view->resetTrackball();
            break;
        case GLFW_KEY_S://Make the drone slower
            view->changePropellerSpeed(-1);
            break;
        case GLFW_KEY_F://Make the drone faster
            view->changePropellerSpeed(1);
            break;
        case GLFW_KEY_Z://Make the drone do a barrel roll
            view->startRotation();
            break;
        case GLFW_KEY_LEFT://rotate the drone left
            view->rotateDrone(1.0f, 0.0f);
            break;
        case GLFW_KEY_RIGHT://rotate the drone right
            view->rotateDrone(-1.0f, 0.0f);
            break;
        case GLFW_KEY_UP://rotate the drone upwards
            view->rotateDrone(0.0f, -1.0f);
            break;
        case GLFW_KEY_DOWN://rotate the drone downwards
            view->rotateDrone(0.0f, 1.0f);
            break;
        case GLFW_KEY_EQUAL://translate the drone forward
            view->moveDrone(1);
            break;
        case GLFW_KEY_MINUS://translate the drone backward
            view->moveDrone(-1);
            break;
        case GLFW_KEY_D://reset the drone to it's original orientation
            view->setDroneOrientation(glm::translate(glm::mat4(1.0f), glm::vec3(-100.0f, 100.0f, 150.0f)));
            break;
        case GLFW_KEY_1:
            view->changeCameraType(1);//Default camera
            break;
        case GLFW_KEY_2:
            view->changeCameraType(2);//Chopper camera
            break;
        case GLFW_KEY_3:
            view->changeCameraType(3);//Drone camera
            break;
        case GLFW_KEY_T:
            view->switchShaders();
            break;
    }
}

void Controller::onMouseInput(int button, int action, int mods)
{
    if(button != GLFW_MOUSE_BUTTON_RIGHT)
        return;
    mousePressed = action == GLFW_PRESS;
    string mouseStatus = mousePressed ? "mouse pressed!" : "mouse released!";
    double xPos, yPos;
    cout<<mouseStatus<<endl;
}

void Controller::onCursorMove(double newXPos, double newYPos)
{
    oldXPos = this->newXPos;
    oldYPos = this->newYPos;
    this->newXPos = newXPos;
    this->newYPos = newYPos;
    float deltaX = newXPos - oldXPos;
    float deltaY = newYPos - oldYPos;
    if(!(mousePressed && ( deltaX != 0 || deltaY != 0)))
    return;
    view->updateTrackball(deltaX, deltaY);
}

void Controller::reshape(int width, int height) 
{
    cout <<"Window reshaped to width=" << width << " and height=" << height << endl;
    glViewport(0, 0, width, height);
    view->Resize();
}

void Controller::dispose()
{
    view->closeWindow();
}

void Controller::error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}