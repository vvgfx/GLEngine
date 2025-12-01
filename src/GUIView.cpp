#include "GUIView.h"
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
#include "sgraph/ScenegraphGUIRenderer.h"
#include "sgraph/NodeDetailsRenderer.h"
#include "VertexAttrib.h"
#include "Pipeline/ShadowVolumePipeline.h"
#include "Pipeline/ShadowVolumeBumpMappingPipeline.h"
#include "Pipeline/BasicPBRPipeline.h"
#include "Pipeline/TexturedPBRPipeline.h"
#include "Pipeline/PBRShadowVolumePipeline.h"
#include "Pipeline/TexturedPBRSVPipeline.h"
#include "sgraph/Jobs/InsertGroupJob.h"
#include "sgraph/Jobs/InsertRotateJob.h"
#include "sgraph/Jobs/InsertTranslateJob.h"
#include "sgraph/Jobs/InsertScaleJob.h"
#include "sgraph/Jobs/InsertLeafJob.h"
#include "sgraph/Jobs/InsertSRTJob.h"
#include "sgraph/Jobs/DeleteNodeJob.h"
#include "sgraph/Jobs/ReadTextureJob.h"
#include "sgraph/Jobs/AddMeshJob.h"
#include "Camera/ICamera.h"
#include "Camera/AngleCamera.h"
#include "Camera/DynamicCamera.h"

// imgui required files.
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

GUIView::GUIView()
{
    resetPopupVars();
}

GUIView::~GUIView()
{
}

void GUIView::init(Callbacks *callbacks, map<string, util::PolygonMesh<VertexAttrib>> &meshes, map<string, unsigned int>& texIdMap, sgraph::IScenegraph* sgraph)
{
    View::init(callbacks, meshes, texIdMap, sgraph);
    cout<<"Setting up ImGUI initialization"<<endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io; // suppress compiler warnings
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // enable keyboard navigation

    ImGui::StyleColorsDark(); // dark mode
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    GuiVisitor = new sgraph::ScenegraphGUIRenderer(this);
    NodeRenderer = new sgraph::NodeDetailsRenderer(this);
    camera = new camera::AngleCamera(glm::vec3(0.0f, 0.0f, 100.0f));

    // NOTE: This REQUIRES a DynamicTransform node called camera that's attached to the root of the scenegraph. In case this exact condition is not met,  the camera will not work.
    // camera = new camera::DynamicCamera(glm::vec3(0.0f, 0.0f, 100.0f), reinterpret_cast<sgraph::DynamicTransform*>(cachedNodes["camera"]));


}

void GUIView::toggleMoveLight()
{
    boolMoveLight = !boolMoveLight;
}

void GUIView::computeTangents(util::PolygonMesh<VertexAttrib> &tmesh)
{
    View::computeTangents(tmesh);
}

void GUIView::Resize()
{
    View::Resize();
}

void GUIView::updateTrackball(float deltaX, float deltaY)
{
    View::updateTrackball(deltaX, deltaY);
}

void GUIView::resetTrackball()
{
    View::resetTrackball();
}

void GUIView::display(sgraph::IScenegraph *scenegraph)
{
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;


    #pragma region lightSetup
    // setting up the view matrices beforehand because all render calculations are going to be on the view coordinate system.

    glm::mat4 viewMat(1.0f);
        viewMat = camera->GetViewMatrix();
    #pragma endregion


    #pragma region pipeline

    pipeline->drawFrame(scenegraph, viewMat);

    #pragma endregion
    

    // Draw GUI here
    if(showGui)
        ImGUIView(scenegraph);

    if(boolMoveLight)
        moveLight();
    
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


void GUIView::guiSwitch()
{
    showGui = !showGui;
}

void GUIView::ImGUIView(sgraph::IScenegraph *scenegraph)
{
    ImGuiIO& io = ImGui::GetIO(); (void)io; // change this later

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("GUI Test!");

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            // ShowExampleMenuFile();
            if (ImGui::MenuItem("Save scene")) 
            {
                showSaveScenePopup = true;
            }
            if (ImGui::MenuItem("Load scene")) 
            {
                showLoadScenePopup = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Task"))
        {
            if (ImGui::MenuItem("Load texture")) 
            {
                loadTexture = true;
            }
            if (ImGui::MenuItem("Load model")) 
            {
                loadModel = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if(showLoadScenePopup)
    {
        ImGui::OpenPopup("Load scene");
        if(ImGui::BeginPopupModal("Load scene"))
        {
            ImGui::InputText("file name", newFileName, 200);
            ImGui::Text("You will lose all unsaved progress.");
            ImGui::Separator();
            if (ImGui::Button("Load"))
            {
                callbacks->loadScene(newFileName);
                ImGui::CloseCurrentPopup();
                resetPopupVars();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) 
            {
                ImGui::CloseCurrentPopup();
                resetPopupVars();
            }
            ImGui::EndPopup();
        }
    }

    if(showSaveScenePopup)
    {
        ImGui::OpenPopup("Save scene");
        if(ImGui::BeginPopupModal("Save scene"))
        {
            ImGui::InputText("file name", saveFileName, 200);
            ImGui::Separator();
            if (ImGui::Button("Save"))
            {
                callbacks->saveScene(saveFileName);
                ImGui::CloseCurrentPopup();
                resetPopupVars();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) 
            {
                ImGui::CloseCurrentPopup();
                resetPopupVars();
            }
            ImGui::EndPopup();
        }
    }

    if(loadTexture)
    {

        ImGui::OpenPopup("Load Texture");
        if(ImGui::BeginPopupModal("Load Texture"))
        {
            ImGui::InputText("Texture name", texName, 100);
            ImGui::InputText("Texture path", texPath, 100);
            ImGui::Separator();
            if (ImGui::Button("Load"))
            {
                job::ReadTextureJob* readJob = new job::ReadTextureJob(texName, texPath);
                getViewJob(readJob);
                ImGui::CloseCurrentPopup();
                resetPopupVars();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) 
            {
                ImGui::CloseCurrentPopup();
                resetPopupVars();
            }
            ImGui::EndPopup();
        }
    }

    /**
     * Not adding models because of complexity for now. Might come back to this later.
     */
    
    if(loadModel)
    {

        ImGui::OpenPopup("Load Model");
        if(ImGui::BeginPopupModal("Load Model"))
        {
            ImGui::InputText("Model name", modelName, 100);
            ImGui::InputText("Model path", modelPath, 100);
            ImGui::Separator();
            if (ImGui::Button("Load"))
            {
                // need to run on the main thread unfortunately :(
                job::AddMeshJob* meshJob = new job::AddMeshJob(modelName, modelPath, callbacks);
                getViewJob(meshJob);
                ImGui::CloseCurrentPopup();
                resetPopupVars();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) 
            {
                ImGui::CloseCurrentPopup();
                resetPopupVars();
            }
            ImGui::EndPopup();
        }
    }

    GUIScenegraph(scenegraph);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GUIView::GUIScenegraph(sgraph::IScenegraph *scenegraph)
{

    // Draw the scenegraph on the left
    ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x / 8 , ImGui::GetIO().DisplaySize.y - 16), ImGuiCond_Always);
    ImGui::Begin("Scenegraph");
    scenegraph->getRoot()->accept(GuiVisitor);
    ImGui::End();

    // Draw the Node details on the right
    sgraph::SGNode* selectedNode = reinterpret_cast<sgraph::ScenegraphGUIRenderer*>(GuiVisitor)->getSelectedNode();
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x  * 5 / 6, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x / 6 , ImGui::GetIO().DisplaySize.y - 16), ImGuiCond_Always);
    ImGui::Begin("Node Details");
    if(selectedNode)
    selectedNode->accept(NodeRenderer);

    // These are for the popups. At any point, I expect only one at most to have a popup
    ImGui::End();

    showPopups();

}

/**
 * This is ugly. Is there an alternative to this method?
 */
void GUIView::showPopups()
{
    sgraph::ScenegraphGUIRenderer* sgRenderer = reinterpret_cast<sgraph::ScenegraphGUIRenderer*>(GuiVisitor);
    sgraph::SGNode* deleteNode = sgRenderer->getDeleteNode();
    if(deleteNode!= nullptr)
    {
        ImGui::OpenPopup("Delete Node?");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if(ImGui::BeginPopupModal("Delete Node?"))
        {
            ImGui::Text("This node will be deleted.\nThis operation cannot be undone!");
            ImGui::Separator();
            if (ImGui::Button("Yes")) 
            {
                job::DeleteNodeJob* deleteNodeJob = new job::DeleteNodeJob(deleteNode->getName(), deleteNode->getParent()->getName());
                getViewJob(deleteNodeJob);
                ImGui::CloseCurrentPopup();
                sgRenderer->resetDeleteNode();
                resetPopupVars();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("No")) 
            {
                ImGui::CloseCurrentPopup();
                sgRenderer->resetDeleteNode();
                resetPopupVars();
            }
            ImGui::EndPopup();
        }
    }

    sgraph::SGNode* addChildNode = sgRenderer->getAddChildNode();
    if(addChildNode != nullptr)
    {
        // show popup for add child
        string childType = sgRenderer->getNodeType();
        if(childType == "Translate")
        {
            ImGui::OpenPopup("Add Translate Node");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            

            if(ImGui::BeginPopupModal("Add Translate Node"))
            {
                ImGui::InputText("Node name", childNodeName, 32);
                float translateFloat3f[3] = {newTranslation.x, newTranslation.y, newTranslation.z};
                bool translateChanged = ImGui::DragFloat3("Translate", translateFloat3f);
                if (translateChanged)
                {
                    newTranslation.x = translateFloat3f[0];
                    newTranslation.y = translateFloat3f[1];
                    newTranslation.z = translateFloat3f[2];
                }
                ImGui::Separator();
                if (ImGui::Button("Confirm")) 
                {
                    job::InsertTranslateJob* translateJob = new job::InsertTranslateJob( sgRenderer->getAddChildNode()->getName() , childNodeName, newTranslation.x, newTranslation.y, newTranslation.z);
                    getViewJob(translateJob);
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) 
                {
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::EndPopup();
            }
        }
        else if(childType == "Rotate")
        {
            ImGui::OpenPopup("Add Rotate Node");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            

            if(ImGui::BeginPopupModal("Add Rotate Node"))
            {
                ImGui::InputText("Node name", childNodeName, 32);
                float newRotation3f[3] = {newRotation.x, newRotation.y, newRotation.z};
                bool rotateChanged = ImGui::DragFloat3("Axis", newRotation3f);
                bool rotChanged = ImGui::InputFloat("Rotation", &newRot, 0.1f, 1.0f);
                if (rotateChanged)
                {
                    newRotation.x = newRotation3f[0];
                    newRotation.y = newRotation3f[1];
                    newRotation.z = newRotation3f[2];
                }
                ImGui::Separator();
                if (ImGui::Button("Confirm")) 
                {
                    job::InserteRotateJob* rotateJob = new job::InserteRotateJob(sgRenderer->getAddChildNode()->getName(), childNodeName, newRotation.x, newRotation.y, newRotation.z, newRot);
                    getViewJob(rotateJob);
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) 
                {
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::EndPopup();
            }
        }
        else if(childType == "Scale")
        {
            ImGui::OpenPopup("Add Scale Node");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            

            if(ImGui::BeginPopupModal("Add Scale Node"))
            {
                ImGui::InputText("Node name", childNodeName, 32);
                float scaleFloat3f[3] = {newScale.x, newScale.y, newScale.z};
                bool scaleChanged = ImGui::DragFloat3("Scale", scaleFloat3f);
                if (scaleChanged)
                {
                    newScale.x = scaleFloat3f[0];
                    newScale.y = scaleFloat3f[1];
                    newScale.z = scaleFloat3f[2];
                }
                ImGui::Separator();
                if (ImGui::Button("Confirm")) 
                {
                    job::InsertScaleJob* scaleJob = new job::InsertScaleJob(sgRenderer->getAddChildNode()->getName(), childNodeName, newScale.x, newScale.y, newScale.z);
                    getViewJob(scaleJob);
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) 
                {
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::EndPopup();
            }
        }
        else if(childType == "SRT")
        {
            ImGui::OpenPopup("Add SRT Node");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            

            if(ImGui::BeginPopupModal("Add SRT Node"))
            {
                ImGui::InputText("Node name", childNodeName, 32);
                float scaleFloat3f[3] = {newScale.x, newScale.y, newScale.z};
                bool scaleChanged = ImGui::DragFloat3("Scale", scaleFloat3f);
                if (scaleChanged)
                {
                    newScale.x = scaleFloat3f[0];
                    newScale.y = scaleFloat3f[1];
                    newScale.z = scaleFloat3f[2];
                }
                float newRotation3f[3] = {newRotation.x, newRotation.y, newRotation.z};
                bool rotateChanged = ImGui::DragFloat3("Rotation", newRotation3f);
                if (rotateChanged)
                {
                    newRotation.x = newRotation3f[0];
                    newRotation.y = newRotation3f[1];
                    newRotation.z = newRotation3f[2];
                }

                float translateFloat3f[3] = {newTranslation.x, newTranslation.y, newTranslation.z};
                bool translateChanged = ImGui::DragFloat3("Translate", translateFloat3f);
                if (translateChanged)
                {
                    newTranslation.x = translateFloat3f[0];
                    newTranslation.y = translateFloat3f[1];
                    newTranslation.z = translateFloat3f[2];
                }
                ImGui::Separator();
                if (ImGui::Button("Confirm")) 
                {
                    job::InsertSRTJob* srtJob = new job::InsertSRTJob(sgRenderer->getAddChildNode()->getName(), childNodeName, newScale.x, newScale.y, newScale.z,
                                                                                            newRotation.x, newRotation.y, newRotation.z, newTranslation.x, newTranslation.y, newTranslation.z);
                    getViewJob(srtJob);
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) 
                {
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::EndPopup();
            }
        }
        else if(childType == "Group")
        {
            ImGui::OpenPopup("Add Group Node");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if(ImGui::BeginPopupModal("Add Group Node"))
            {
                ImGui::InputText("Node name", childNodeName, 32);
                ImGui::Separator();
                if (ImGui::Button("Confirm")) 
                {
                    job::InsertGroupJob* groupJob = new job::InsertGroupJob(sgRenderer->getAddChildNode()->getName(), childNodeName);
                    getViewJob(groupJob);
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) 
                {
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::EndPopup();
            }
        }
        else if(childType == "Leaf")
        {
            ImGui::OpenPopup("Add Leaf Node");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if(ImGui::BeginPopupModal("Add Leaf Node"))
            {
                ImGui::InputText("Node name", childNodeName, 32);
                ImGui::InputText("Object instance", objectInstanceName, 32);

                ImVec4 colorAlbedo = ImVec4(leafAlbedo.x, leafAlbedo.y, leafAlbedo.z, 1.0f);
                if(ImGui::ColorEdit3("albedo", (float*)&colorAlbedo))
                {
                    leafAlbedo.x = colorAlbedo.x;
                    leafAlbedo.y = colorAlbedo.y;
                    leafAlbedo.z = colorAlbedo.z;
                };

                bool metallicChanged = ImGui::SliderFloat("Metallic", &materialMetallic, 0.1f, 1.0f);
                bool roughnessChanged = ImGui::SliderFloat("Roughness", &materialRoughness, 0.1f, 1.0f);
                bool aoChanged = ImGui::SliderFloat("Ambient Occlusion", &materialAO, 0.1f, 1.0f);
                
                ImGui::Separator();

                if(ImGui::Checkbox("Textures", &leafTextures))
                {
                }
                if(leafTextures)
                {
                    ImGui::InputText("Albedo map", albedoMap, 100);
                    ImGui::InputText("Normal map", normalMap, 100);
                    ImGui::InputText("Metallic map", metallicMap, 100);
                    ImGui::InputText("Roughness map", roughnessMap, 100);
                    ImGui::InputText("AO map", aoMap, 100);
                }

                if (ImGui::Button("Confirm")) 
                {
                    job::InsertLeafJob* leafJob = new job::InsertLeafJob(sgRenderer->getAddChildNode()->getName(), childNodeName, leafAlbedo, materialMetallic, materialRoughness, materialAO, objectInstanceName,
                                                                            leafTextures, albedoMap, normalMap, metallicMap, roughnessMap, aoMap);
                    getViewJob(leafJob);
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) 
                {
                    ImGui::CloseCurrentPopup();
                    sgRenderer->resetAddChildNode();
                    resetPopupVars();
                }
                ImGui::EndPopup();
            }
        }
    }
}

void GUIView::resetPopupVars()
{
    strcpy(childNodeName, ""); // reset char array
    newTranslation = glm::vec3(0.0f);
    newRotation = glm::vec3(0.0f);
    newScale = glm::vec3(0.0f);
    newRot = 0.0f;

    strcpy(objectInstanceName, ""); // reset char array
    strcpy(materialName, ""); // reset char array
    leafAlbedo = glm::vec3(0.0f);
    materialMetallic = 0.0f;
    materialRoughness = 0.0f;
    materialAO = 0.0f;

    // texture stuff here
    leafTextures = false;
    strcpy(albedoMap , "");
    strcpy(normalMap , "");
    strcpy(metallicMap , "");
    strcpy(roughnessMap , "");
    strcpy(aoMap , "");

    // main menu stuff here
    loadTexture = false;
    strcpy(texName, "");
    strcpy(texPath, "");

    // model load here
    loadModel = false;
    strcpy(modelName, "");
    strcpy(modelPath, "");

    // new file stuff here
    strcpy(newFileName, "");
    showLoadScenePopup = false;

    // save scene here
    strcpy(saveFileName, "");
    showSaveScenePopup = false;
}

void GUIView::initLights(sgraph::IScenegraph *scenegraph)
{
    View::initLights(scenegraph);
}

void GUIView::initLightShaderVars()
{
    View::initLightShaderVars();
}

bool GUIView::shouldWindowClose()
{
    return View::shouldWindowClose();
}

void GUIView::switchShaders()
{
    // not supported
}

void GUIView::closeWindow()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    View::closeWindow();
}

// This saves all the nodes required for dynamic transformation
void GUIView::initScenegraphNodes(sgraph::IScenegraph *scenegraph)
{
    View::initScenegraphNodes(scenegraph);

    // save root node for light movement.
    auto nodes = scenegraph->getNodes();
    if(nodes->find("root") != nodes->end())
    {
        cout<<"Found root for light!: "<<endl;
        lightNode = dynamic_cast<sgraph::GroupNode*>((*nodes)["root"]);
    }
}

void GUIView::moveLight()
{
    vector<util::Light>* lights = lightNode->getLights();
    if((*lights).size() > 0)
    {
        float time = glfwGetTime();
        glm::vec4 currentPos = (*lights)[0].getPosition();
        float xLoc = tanh(2 * cos(time * 2)) * 15;
        (*lights)[0].setPosition(xLoc, currentPos.y, currentPos.z);
    }

}

void GUIView::rotatePropeller(string nodeName, float time)
{
    // not supported
}

void GUIView::changePropellerSpeed(int num)
{
    // not supported
}

void GUIView::startRotation()
{
    // not supported
}

void GUIView::rotate()
{
    // not supported
}

/**
 * Move/Rotate the drone by passing the matrix to postmultiply. This stacks on top of previous input.
 * 
 * Update: Using this for camera movement
 */
void GUIView::moveDrone(int direction)
{
    // not supported
}

/**
 * Set the drone's matrix to what is passed. A good idea would be to pass a rotation and a translation.
 */
void GUIView::setDroneOrientation(glm::mat4 resetMatrix)
{
    // not supported
}

/**
 * Pass a positive yawDir to rotate left, negative to rotate right. Similarly, positive pitchDir to rotate upwards, negative to rotate downwards
 */

void GUIView::rotateDrone(int yawDir, int pitchDir)
{
    // not supported
}

void GUIView::changeCameraType(int cameraType)
{
    // not supported
}

void GUIView::getViewJob(job::IJob* job)
{
    callbacks->receiveJob(job);
}

void GUIView::setGUICallbackReference(GUICallbacks* callbacks)
{
    this->callbacks = callbacks;
}

void GUIView::moveCamera(int forwardDir, int horizontalDir)
{
    cout<<"Moving camera!!"<<endl;
    if(forwardDir > 0)
        camera->ProcessKeyboard(FORWARD, deltaTime);
    else if(forwardDir < 0)
        camera->ProcessKeyboard(BACKWARD, deltaTime);
    if(horizontalDir > 0)
        camera->ProcessKeyboard(RIGHT,deltaTime);
    if(horizontalDir <  0)
        camera->ProcessKeyboard(LEFT, deltaTime);


}

// yawDir = deltaX, pitchDir = deltaY
void GUIView::rotateCamera(int yawDir, int pitchDir)
{
    camera->ProcessMouseMovement(yawDir, -pitchDir);
}

void GUIView::loadMesh(string meshName, util::PolygonMesh<VertexAttrib>& polymesh)
{
    pipeline->addMesh(meshName, polymesh);
}

void GUIView::loadCubeMaps(vector<util::TextureImage*>& cubeMap)
{
    pipeline->loadCubeMap(cubeMap);
}

void GUIView::pipelineCallbacks(int key)
{
    pipeline->keyCallback(key);
}