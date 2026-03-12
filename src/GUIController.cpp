#include "GUIController.h"
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
#include <thread>
#include <fstream>
using namespace std;

#include "sgraph/ScenegraphExporter.h"
#include "sgraph/ScenegraphImporter.h"
#include "sgraph/ScenegraphDrawer.h"
#include "GUIView.h"
#include "imgui.h"

// restart imports here
#ifdef _WIN32
    #include <process.h>
#else
    #include <unistd.h>
#endif


GUIController::GUIController(Model* m,View* v, string textfile) :
    Controller(m,v, textfile) {
        wFlag = sFlag = aFlag = dFlag = false;
}

void GUIController::initScenegraph() {

     
    
    //read in the file of commands
    ifstream inFile;
    if(textfile == "")
        inFile = ifstream("scenegraphmodels/test-export-2.txt");
    else
        inFile = ifstream(textfile);
    sgraph::ScenegraphImporter importer;
    

    IScenegraph *scenegraph = importer.parse(inFile);
    //scenegraph->setMeshes(meshes);
    model->setScenegraph(scenegraph);
    map<string, util::TextureImage*> textureMap = importer.getTextureMap(); // this is copied

    map<string, string> texturePaths = importer.getTexturePaths();

    model->saveTextureMap(textureMap); // this should also be copied, the source of truth is the model!

    model->saveTexturePaths(texturePaths);
    // now save the cubemap references
    model->saveCubeMapTextures(importer.getCubeMap());
    model->saveCubeMapTexPaths(importer.getCubeMapPaths());
    // ugly code, can fix later??
    reinterpret_cast<GUIView*>(view)->setGUICallbackReference(this);

    cout <<"Scenegraph made in GUIController" << endl;   
    sgraph::ScenegraphDrawer* drawer = new sgraph::ScenegraphDrawer();
    scenegraph->getRoot()->accept(drawer);
}

GUIController::~GUIController()
{
    
}

void GUIController::run()
{
    sgraph::IScenegraph * scenegraph = model->getScenegraph();
    map<string,util::PolygonMesh<VertexAttrib> > meshes = scenegraph->getMeshes();

    map<string, unsigned int>& texIdMap = model->getTextureIdMap(); // not copied! reference to the model variable.
    view->init(this,meshes, texIdMap, scenegraph);

    // initialize the cubemap as well!
    vector<util::TextureImage*>& cubeMaps = model->getCubeMapTextures();
    if(!(cubeMaps.size() < 1))
        reinterpret_cast<GUIView*>(view)->loadCubeMaps(cubeMaps);

    // creating the texture Id maps AFTER init. This is because the OpenGL initialization needs to occur before the textures can be loaded
    model->initTextures(model->getTextureMap());
    //Save the nodes required for transformation when running!
    // view->initScenegraphNodes(scenegraph);
    while (!view->shouldWindowClose()) {
        processInputs();
        model->clearQueues();
        view->display(scenegraph);
    }
    view->closeWindow();
    exit(EXIT_SUCCESS);
}

void GUIController::onkey(int key, int scancode, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureKeyboard)
        return;
    cout << (char)key << " pressed on GUIController" << endl;
    if(action == GLFW_PRESS)
    {
        switch(key)
        {
            case GLFW_KEY_W:
                wFlag = true;
                break;
            case GLFW_KEY_S:
                sFlag = true;
                break;
            case GLFW_KEY_A:
                aFlag = true;
                break;
            case GLFW_KEY_D:
                dFlag = true;
                break;
            case GLFW_KEY_LEFT://rotate the drone left
                reinterpret_cast<GUIView*>(view)->rotateCamera(1.0f, 0.0f);
                break;
            case GLFW_KEY_RIGHT://rotate the drone right
                reinterpret_cast<GUIView*>(view)->rotateCamera(-1.0f, 0.0f);
                break;
            case GLFW_KEY_0:
                    saveScene("scenegraphmodels/test-export.txt");
                break;
            case GLFW_KEY_G:
                    reinterpret_cast<GUIView*>(view)->guiSwitch();
                break;
            case GLFW_KEY_1:
                    reinterpret_cast<GUIView*>(view)->pipelineCallbacks(key);
                break;
            case GLFW_KEY_2:
                    reinterpret_cast<GUIView*>(view)->toggleMoveLight();
                break;
            case GLFW_KEY_3:
                    reinterpret_cast<GUIView*>(view)->pipelineCallbacks(key);
                break;
        }
    }
    else if(action == GLFW_RELEASE)
    {
        switch(key)
        {
            case GLFW_KEY_W:
                wFlag = false;
                break;
            case GLFW_KEY_S:
                sFlag = false;
                break;
            case GLFW_KEY_A:
                aFlag = false;
                break;
            case GLFW_KEY_D:
                dFlag = false;
                break;
        }
    }
}

void GUIController::processInputs()
{
    if(wFlag)
        reinterpret_cast<GUIView*>(view)->moveCamera(1, 0);
    if(sFlag)
        reinterpret_cast<GUIView*>(view)->moveCamera(-1, 0);
    if(aFlag)
        reinterpret_cast<GUIView*>(view)->moveCamera(0, -1);
    if(dFlag)
        reinterpret_cast<GUIView*>(view)->moveCamera(0, 1);
}

void GUIController::onMouseInput(int button, int action, int mods)
{
    Controller::onMouseInput(button, action, mods);
}

void GUIController::onCursorMove(double newXPos, double newYPos)
{
    oldXPos = this->newXPos;
    oldYPos = this->newYPos;
    this->newXPos = newXPos;
    this->newYPos = newYPos;
    float deltaX = newXPos - oldXPos;
    float deltaY = newYPos - oldYPos;
    if(!(mousePressed && ( deltaX != 0 || deltaY != 0)))
    return;
    reinterpret_cast<GUIView*>(view)->rotateCamera(deltaX, deltaY);
}

void GUIController::reshape(int width, int height) 
{
    Controller::reshape(width, height);
}

void GUIController::dispose()
{
    Controller::dispose();
}

void GUIController::error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void GUIController::receiveJob(job::IJob* job)
{
    cout<<"received job in controller"<<endl;
    // job->execute(model);
    thread t([this, job]()
    {
        job->execute(model);
        delete job;
    });
    t.detach();
    
}

void GUIController::loadScene(string newScenegraphName)
{
    view->closeWindow();
    glfwTerminate();
    vector<char*> args;
    args.push_back(const_cast<char*>("rendering_engine.exe"));
    args.push_back(const_cast<char*>(newScenegraphName.c_str()));
    args.push_back(nullptr);             

    #ifdef _WIN32
        _execv("Rendering_Engine.exe", args.data());  // Windows
    #else
        execv("./Rendering_Engine", args.data());   // Unix/Linux/macOS
    #endif
}

void GUIController::saveScene(string name)
{
    ofstream file(name);
    sgraph::ScenegraphExporter* exporter = new sgraph::ScenegraphExporter(model->getScenegraph()->getMeshPaths(), model->getTexturePaths(), model->getCubeMapTexPaths());
    model->getScenegraph()->getRoot()->accept(exporter);
    file << exporter->getOutput();
    file.close();
}

void GUIController::loadMesh(string meshName, util::PolygonMesh<VertexAttrib>& polymesh)
{
    reinterpret_cast<GUIView*>(view)->loadMesh(meshName, polymesh);
}