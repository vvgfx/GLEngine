#ifndef _SCENEGRAPHGUIRENDERER_H_
#define _SCENEGRAPHGUIRENDERER_H_

#include "SGNodeVisitor.h"
#include "GroupNode.h"
#include "LeafNode.h"
#include "TransformNode.h"
#include "RotateTransform.h"
#include "ScaleTransform.h"
#include "TranslateTransform.h"
#include "DynamicTransform.h"
#include "SRTNode.h"
#include <ShaderProgram.h>
#include <ShaderLocationsVault.h>
#include "ObjectInstance.h"
#include "Light.h"
#include "imgui.h"
#include <stack>
#include <iostream>
#include "Jobs/InsertTranslateJob.h"
#include "Jobs/InsertRotateJob.h"
#include "Jobs/InsertScaleJob.h"
using namespace std;

namespace sgraph
{
    /**
     * This visitor implements drawing the GUI graph using ImGUI
     *
     */
    class ScenegraphGUIRenderer : public SGNodeVisitor
    {
    public:
        /**
         * @brief Construct a new ScenegraphGUIRenderer object
         *
         * @param mv a reference to modelview stack that will be used to convert light to the view co-ordinate system
         */
        ScenegraphGUIRenderer(GUIView *v) : view(v) {   
            selectedNode = nullptr;
            deleteNode = nullptr;
            addChildNode = nullptr;
            nodeType = "";
        }

        /**
         * @brief Visit parent node to draw GUI.
         *
         * @param groupNode
         */
        void visitGroupNode(GroupNode *groupNode)
        {
            visitParentNode(groupNode, true);
        }

        /**
         * @brief draw leaf node on GUI.
         * 
         *
         * @param leafNode
         */
        void visitLeafNode(LeafNode *leafNode)
        {
            ImGuiTreeNodeFlags flags = (selectedNode == leafNode) ? ImGuiTreeNodeFlags_Selected : 0;
            flags = flags | ImGuiTreeNodeFlags_Leaf ;
            bool nodeOpen = ImGui::TreeNodeEx(leafNode->getName().c_str(), flags);
            if (ImGui::IsItemClicked())
                selectedNode = leafNode;
            if(nodeOpen)
                ImGui::TreePop();
        }

        /**
         * @brief Draw node on GUI.
         *
         * @param parentNode
         */
        void visitParentNode(ParentSGNode *parentNode, bool enabledFlag)
        {
            ImGuiTreeNodeFlags flags = (selectedNode == parentNode) ? ImGuiTreeNodeFlags_Selected : 0;
            bool nodeOpen = ImGui::TreeNodeEx(parentNode->getName().c_str(), flags);
            if (ImGui::IsItemClicked())
                selectedNode = parentNode;
            RightClickMenu(enabledFlag, parentNode);
            if(nodeOpen)
            {
                if (parentNode->getChildren().size() > 0)
                {
                    // parentNode->getChildren()[0]->accept(this);
                    for (int i = 0; i < parentNode->getChildren().size(); i = i + 1)
                    {
                        parentNode->getChildren()[i]->accept(this);
                    }
                }
                ImGui::TreePop();
            }
        }

        /**
         * @brief Visit parent node to draw GUI.
         *
         * @param transformNode
         */
        void visitTransformNode(TransformNode *transformNode)
        {
            visitParentNode(transformNode, !(transformNode->getChildren().size() > 0));
        }

        /**
         * @brief Visit parent node to draw GUI.
         *
         * @param scaleNode
         */
        void visitScaleTransform(ScaleTransform *scaleNode)
        {
            visitParentNode(scaleNode, !(scaleNode->getChildren().size() > 0));
        }

        /**
         * @brief Visit parent node to draw GUI.
         *
         * @param translateNode
         */
        void visitTranslateTransform(TranslateTransform *translateNode)
        {
            visitParentNode(translateNode, !(translateNode->getChildren().size() > 0));
        }


        /**
         * @brief Visit parent node to draw GUI.
         *
         * @param rotateNode
         */
        void visitRotateTransform(RotateTransform *rotateNode)
        {
            visitParentNode(rotateNode, !(rotateNode->getChildren().size() > 0));
        }

        /**
         * @brief Visit parent node to draw GUI.
         *
         * @param dynamicTransformNode
         */
        void visitDynamicTransform(DynamicTransform *dynamicTransformNode)
        {
            visitParentNode(dynamicTransformNode, !(dynamicTransformNode->getChildren().size() > 0));
        }

        /**
         * @brief Visit parent node to draw GUI.
         *
         * @param srtNode
         */
        void visitSRTNode(SRTNode *srtNode)
        {
            visitParentNode(srtNode, !(srtNode->getChildren().size() > 0));
        }

        /**
         * @brief Get the node selected by the user.
         *
         */
        SGNode* getSelectedNode()
        {
            return selectedNode;
        }

        
        /**
         * This method draws the right click menu for all scenegraph nodes
         */
        void RightClickMenu(bool AddChildEnabled, SGNode* node)
        {
            if(ImGui::BeginPopupContextItem())
            {
                if (ImGui::BeginMenu("Add Child", AddChildEnabled))
                {
                    if(ImGui::MenuItem("Group"))
                    {
                        addChildNode = node;
                        nodeType = "Group";
                    }
                    if(ImGui::MenuItem("SRT"))
                    {
                        addChildNode = node;
                        nodeType = "SRT";
                    }
                    if(ImGui::MenuItem("Translate"))
                    {
                        addChildNode = node;
                        nodeType = "Translate";
                    }
                    if(ImGui::MenuItem("Rotate"))
                    {
                        addChildNode = node;
                        nodeType = "Rotate";
                    }
                    if(ImGui::MenuItem("Scale"))
                    {
                        addChildNode = node;
                        nodeType = "Scale";
                    }
                    if(ImGui::MenuItem("Leaf"))
                    {
                        addChildNode = node;
                        nodeType = "Leaf";
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::MenuItem("Delete Node"))
                {
                    deleteNode = node;
                }
                ImGui::EndPopup();
            }
        }


        SGNode* getDeleteNode()
        {
            return deleteNode;
        }

        SGNode* getAddChildNode()
        {
            return addChildNode;
        }

        string getNodeType()
        {
            return nodeType;
        }

        void resetNodeType()
        {
            nodeType = "";
        }

        void resetDeleteNode()
        {
            deleteNode = nullptr;
        }

        void resetAddChildNode()
        {
            addChildNode = nullptr;
            nodeType = "";
        }


    private:

        SGNode* selectedNode;
        GUIView* view;
        SGNode* deleteNode;
        SGNode* addChildNode;
        string nodeType;
    };
}

#endif