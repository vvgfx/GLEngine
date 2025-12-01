#ifndef _NODEDETAILSRENDERER_H_
#define _NODEDETAILSRENDERER_H_

#include "SGNodeVisitor.h"
#include "GroupNode.h"
#include "LeafNode.h"
#include "TransformNode.h"
#include "RotateTransform.h"
#include "ScaleTransform.h"
#include "TranslateTransform.h"
#include "DynamicTransform.h"
#include "SRTNode.h"
#include "Jobs/UpdateScaleJob.h"
#include "Jobs/UpdateTranslateJob.h"
#include "Jobs/UpdateRotateJob.h"
#include "Jobs/UpdateLightJob.h"
#include "Jobs/InsertLightJob.h"
#include "Jobs/DeleteLightJob.h"
#include "Jobs/UpdateLeafMaterialJob.h"
#include "Jobs/UpdateSRTJob.h"
#include "../GUIView.h"
#include <ShaderProgram.h>
#include <ShaderLocationsVault.h>
#include "ObjectInstance.h"
#include "Light.h"
#include "imgui.h"
#include <stack>
#include <iostream>
using namespace std;

namespace sgraph
{
    /**
     * This visitor implements drawing the GUI graph using ImGUI
     *
     */
    class NodeDetailsRenderer : public SGNodeVisitor
    {
    public:
        /**
         * @brief Construct a new NodeDetailsRenderer object
         *
         * @param mv a reference to modelview stack that will be used to convert light to the view co-ordinate system
         */
        NodeDetailsRenderer(GUIView *v) : view(v)
        {
            resetLightPopupVars();
        }

        /**
         * @brief Recur to the children for drawing
         *
         * @param groupNode
         */
        void visitGroupNode(GroupNode *groupNode)
        {
            if (ImGui::CollapsingHeader("Group Node", ImGuiTreeNodeFlags_DefaultOpen))
            {
            }
            drawLightHeader(groupNode);
        }

        /**
         * @brief get lights attached to this leaf(if any)
         *
         *
         * @param leafNode
         */
        void visitLeafNode(LeafNode *leafNode)
        {
            if (ImGui::CollapsingHeader("Leaf Node", ImGuiTreeNodeFlags_DefaultOpen))
            {
                string name = leafNode->getName();
                util::Material leafMaterial = leafNode->getMaterial();
                glm::vec4 albedo = leafMaterial.getAlbedo();
                ImVec4 colorAlbedo =  ImVec4(albedo.x, albedo.y, albedo.z, albedo.w);
                float metallic = leafMaterial.getMetallic();
                float roughness = leafMaterial.getRoughness();
                float ao = leafMaterial.getAO();
                bool changed = false;
                if(ImGui::ColorEdit3("Albedo", (float*)&colorAlbedo))
                {
                    changed = true;
                    albedo = {colorAlbedo.x, colorAlbedo.y, colorAlbedo.z, colorAlbedo.w};
                }
                if(ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
                {
                    changed = true;
                }
                if(ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
                {
                    changed = true;
                }
                if(ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f))
                {
                    changed = true;
                }
                ImGui::Separator();
                ImGui::Text("Textures");
                string albedoMap = leafNode->getTextureMap();
                string normalMap = leafNode->getNormalMap();
                string metallicMap = leafNode->getMetallicMap();
                string roughnessMap = leafNode->getRoughnessMap();
                string aoMap = leafNode->getAOMap();
                ImGui::BeginDisabled(true);
                {
                    ImGui::InputText("Albedo map" , &albedoMap[0], albedoMap.size(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::InputText("Normal map" , &normalMap[0], normalMap.size(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::InputText("Metallic map" , &metallicMap[0], metallicMap.size(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::InputText("Roughness map" , &roughnessMap[0], roughnessMap.size(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::InputText("AO map" , &aoMap[0], aoMap.size(), ImGuiInputTextFlags_ReadOnly);
                }
                ImGui::EndDisabled();
                if(changed)
                {
                    job::UpdateLeafMaterialJob* updateMatJob = new job::UpdateLeafMaterialJob(leafNode->getName(), albedo, metallic, roughness, ao);
                    view->getViewJob(updateMatJob);
                }

            }
            drawLightHeader(leafNode);
        }

        /**
         * @brief Multiply the transform to the modelview and recur to child
         *
         * @param parentNode
         */
        void visitParentNode(ParentSGNode *parentNode)
        {
            if (ImGui::CollapsingHeader("Parent Node", ImGuiTreeNodeFlags_DefaultOpen))
            {
            }
            drawLightHeader(parentNode);
        }

        /**
         * @brief Multiply the transform to the modelview and recur to child
         *
         * @param transformNode
         */
        void visitTransformNode(TransformNode *transformNode)
        {
            if (ImGui::CollapsingHeader("Transform Node", ImGuiTreeNodeFlags_DefaultOpen))
            {
            }
            drawLightHeader(transformNode);
        }

        /**
         * @brief For this visitor, only the transformation matrix is required.
         * Thus there is nothing special to be done for each type of transformation.
         * We delegate to visitParentNode above
         *
         * @param scaleNode
         */
        void visitScaleTransform(ScaleTransform *scaleNode)
        {
            if (ImGui::CollapsingHeader("Scale Node", ImGuiTreeNodeFlags_DefaultOpen))
            {
                glm::vec3 scale = scaleNode->getScale();
                float vec3f[3] = {scale.x, scale.y, scale.z};
                if (ImGui::DragFloat3("Scale", vec3f))
                {
                    // this runs only when a value is changed
                    job::UpdateScaleJob *scaleJob = new job::UpdateScaleJob(scaleNode->getName(), vec3f[0], vec3f[1], vec3f[2]);
                    view->getViewJob(scaleJob);
                }
            }
            drawLightHeader(scaleNode);
        }

        /**
         * @brief For this visitor, only the transformation matrix is required.
         * Thus there is nothing special to be done for each type of transformation.
         * We delegate to visitParentNode above
         *
         * @param translateNode
         */
        void visitTranslateTransform(TranslateTransform *translateNode)
        {
            if (ImGui::CollapsingHeader("Translate Node", ImGuiTreeNodeFlags_DefaultOpen))
            {
                glm::vec3 translate = translateNode->getTranslate();
                float vec3f[3] = {translate.x, translate.y, translate.z};
                if (ImGui::DragFloat3("Translate", vec3f))
                {
                    // runs when value is changed
                    // cout << "value changed in NodeDetailsRenderer" << endl;
                    job::UpdateTranslateJob *translateJob = new job::UpdateTranslateJob(translateNode->getName(), vec3f[0], vec3f[1], vec3f[2]);
                    view->getViewJob(translateJob);
                    
                }
            }
            drawLightHeader(translateNode);
        }

        void visitRotateTransform(RotateTransform *rotateNode)
        {
            if (ImGui::CollapsingHeader("Rotate Node", ImGuiTreeNodeFlags_DefaultOpen))
            {

                glm::vec3 rotation = rotateNode->getRotationAxis();
                float angle = glm::degrees(rotateNode->getAngleInRadians());
                float vec3f[3] = {rotation.x, rotation.y, rotation.z};
                bool changed = ImGui::DragFloat3("Rotate", vec3f);
                bool rotChanged = ImGui::InputFloat("Value", &angle, 0.1f, 1.0f);
                if (changed || rotChanged)
                {
                    job::UpdateRotateJob *rotateJob = new job::UpdateRotateJob(rotateNode->getName(), vec3f[0], vec3f[1], vec3f[2], angle);
                    view->getViewJob(rotateJob);
                }
            }
            drawLightHeader(rotateNode);
        }

        void visitDynamicTransform(DynamicTransform *dynamicTransformNode)
        {
            if (ImGui::CollapsingHeader("Dynamic Node", ImGuiTreeNodeFlags_DefaultOpen))
            {
            }
            drawLightHeader(dynamicTransformNode);
        }


        void visitSRTNode(SRTNode *srtNode)
        {
            if (ImGui::CollapsingHeader("SRT Node", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool changed  = false;
                glm::vec3 scale = srtNode->getScale();
                float vec3fs[3] = {scale.x, scale.y, scale.z};
                if (ImGui::DragFloat3("Scale", vec3fs))
                {
                    changed = true;
                }

                glm::vec3 rotate = srtNode->getRotate();
                float vec3fr[3] = {glm::degrees(rotate.x), glm::degrees(rotate.y), glm::degrees(rotate.z)};
                if (ImGui::DragFloat3("Rotate", vec3fr))
                {
                    changed = true;
                }

                glm::vec3 translate = srtNode->getTranslate();
                float vec3ft[3] = {translate.x, translate.y, translate.z};
                if (ImGui::DragFloat3("Translate", vec3ft))
                {
                    changed = true;
                }
                if(changed)
                {
                    // update SRT Job here.
                    job::UpdateSRTJob* srtJob = new job::UpdateSRTJob(srtNode->getName(), vec3fs[0], vec3fs[1], vec3fs[2], vec3fr[0], vec3fr[1], vec3fr[2], vec3ft[0], vec3ft[1], vec3ft[2]);
                    view->getViewJob(srtJob);
                    changed = false;

                }
            }
            drawLightHeader(srtNode);
        }

        void drawLightHeader(SGNode *node)
        {
            if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
            {
                vector<util::Light> lights = *(node->getLights());

                for (int i = 0; i < lights.size(); i++)
                {
                    ImGui::PushID(i); // Using index as unique ID
                    string lightName = lights[i].getName();
                    glm::vec3 color = lights[i].getColor();
                    float colorFloat[3] = {color.x, color.y, color.z};
                    glm::vec4 spotDirection = lights[i].getSpotDirection();
                    float spotDirFloat[3] = {spotDirection.x, spotDirection.y, spotDirection.z};
                    glm::vec4 position = lights[i].getPosition();
                    float positionFloat[4] = {position.x, position.y, position.z, position.w};
                    float spotAngle = lights[i].getSpotCutoff();
                    bool changed  = false;
                    string lightText = "Light name : "  + lightName;
                    ImGui::Text(lightText.c_str(), lightName.c_str());
                    if (ImGui::DragFloat3("Color", colorFloat))
                    {
                        changed = true;
                    }

                    if (ImGui::DragFloat3("Spot Direction", spotDirFloat))
                    {
                        changed = true;
                    }

                    if (ImGui::DragFloat3("Position", positionFloat))
                    {
                        changed = true;
                    }

                    if (ImGui::InputFloat("Angle", &spotAngle))
                    {
                        changed = true;
                    }
                    if(changed)
                    {
                        job::UpdateLightJob *updateLightJob = new job::UpdateLightJob(node->getName(), lights[i].getName(), colorFloat, spotDirFloat, positionFloat, spotAngle);
                        view->getViewJob(updateLightJob);
                    }
                    if(ImGui::Button("Delete light"))
                    {
                        job::DeleteLightJob *deleteLightJob = new job::DeleteLightJob(node->getName(), lights[i].getName());
                        view->getViewJob(deleteLightJob);
                    }
                    ImGui::PopID();
                    ImGui::Separator();
                }

                // new light logic here
                if(ImGui::Button("Add new light"))
                {
                    showLightPopup = true;
                    addLightNode = node;
                }
                if(showLightPopup)
                {
                    // light popup
                    ImGui::OpenPopup("Add new light");
                    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if(ImGui::BeginPopupModal("Add new light"))
                    {
                        ImGui::InputText("Light name", lightName, 32);
                        float colorFloat3f[3] = {color.x, color.y, color.z};
                        if(ImGui::DragFloat3("Color", colorFloat3f))
                        {
                            color.x = colorFloat3f[0];
                            color.y = colorFloat3f[1];
                            color.z = colorFloat3f[2];
                        }

                        float position3f[3] = {position.x, position.y, position.z};
                        if(ImGui::DragFloat3("Position", position3f))
                        {
                            position.x = position3f[0];
                            position.y = position3f[1];
                            position.z = position3f[2];
                        }

                        float spotDir3f[3] = {spotDir.x, spotDir.y, spotDir.z};
                        if(ImGui::DragFloat3("Spot Direction", spotDir3f))
                        {
                            spotDir.x = spotDir3f[0];
                            spotDir.y = spotDir3f[1];
                            spotDir.z = spotDir3f[2];
                        }

                        if(ImGui::InputFloat("Spot Angle", &spotAngle))
                        {
                        }

                        if (ImGui::Button("Confirm")) 
                        {
                            job::InsertLightJob* insertLightJob = new job::InsertLightJob(addLightNode->getName(), lightName, color, spotDir, position, spotAngle);
                            view->getViewJob(insertLightJob);
                            resetLightPopupVars();
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SetItemDefaultFocus();
                        ImGui::SameLine();
                        if (ImGui::Button("Cancel")) 
                        {
                            resetLightPopupVars();
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }
            }
        }

        void resetLightPopupVars()
        {
            showLightPopup = false;
            strcpy(lightName, "");
            spotDir = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            color = glm::vec3(0.0f);
            spotAngle = 0.0f;
        }

        

        private:
            bool showLightPopup;
            SGNode* addLightNode;
            char lightName[32];
            glm::vec4 spotDir, position;
            glm::vec3 color;
            float spotAngle;
            GUIView *view;
        };
    }

#endif