#ifndef _LEAFNODE_H_
#define _LEAFNODE_H_

#include "AbstractSGNode.h"
#include "SGNodeVisitor.h"
#include "Material.h"
#include "glm/glm.hpp"
#include <map>
#include <stack>
#include <string>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;

namespace sgraph
{

/**
 * This node represents the leaf of a scene graph. It is the only type of node that has
 * actual geometry to render.
 * \author Amit Shesh
 */
class LeafNode: public AbstractSGNode
{
    /**
     * The name of the object instance that this leaf contains. All object instances are stored
     * in the scene graph itself, so that an instance can be reused in several leaves
     */
protected:
    string objInstanceName;
    /**
     * The material associated with the object instance at this leaf
     */
    util::Material material;
    string textureMap; // albedo map
    glm::mat4 textureTransform;
    string normalMap;
    bool isPBR;
    string metallicMap;
    string roughnessMap;
    string aoMap;

public:
    LeafNode(const string& instanceOf,util::Material& material,const string& name,sgraph::IScenegraph *graph, string texName, string normalName)
        :AbstractSGNode(name,graph) {
        this->objInstanceName = instanceOf;
        this->material = material;
        this->textureMap = texName;
        this->normalMap = normalName;
        this->isPBR = true; // this is true if the normalMap is set.
        textureTransform  = glm::mat4(1.0f);//Assuming that the default texture transformation is the identity matrix.
    }

    LeafNode(const string& instanceOf,const string& name,sgraph::IScenegraph *graph)
        :AbstractSGNode(name,graph) {
        this->objInstanceName = instanceOf;
        this->textureMap = "";
        // this->normalMap = normalName;
        this->isPBR = false;
        // this->textureTransform = glm::mat4(1.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f));
        this->textureTransform = glm::mat4(1.0f);
        this->normalMap = "";
        this->metallicMap = "";
        this->roughnessMap = "";
        this->aoMap = "";
    }
	
	~LeafNode(){}



    /*
	 *Set the material of each vertex in this object
	 */
    void setMaterial(const util::Material& mat) {
        material = mat;
    }

    /*
     * gets the material
     */
    util::Material getMaterial()
    {
        return material;
    }

    /**
     * Set the name of the texture corresponding to this leaf.
     */
    void setTextureMap(string texName)
    {
        textureMap = texName;
    }

    /**
     * Get the nameof the texture corresponding to this leaf.
     */
    string getTextureMap()
    {
        return textureMap;
    }


    /**
     * Set the name of the normal texture corresponding to this leaf.
     */
    void setNormalMap(string texName)
    {
        this->isPBR = true;
        cout<<"PBR set to true"<<endl;
        normalMap = texName;
    }

    /**
     * Set the metallic map of this leaf.
     */
    void setMetallicMap(string metalMapName)
    {
        this->isPBR = true;
        this->metallicMap = metalMapName;
    }

    /**
     * Get the metallic map of this leaf.
     */
    string getMetallicMap()
    {
        return this->metallicMap;
    }

    /**
     * Set the roughness map of this leaf.
     */
    void setRoughnessMap(string roughMap)
    {
        this->isPBR = true;
        this->roughnessMap = roughMap;
    }

    /**
     * Get the roughness map of this leaf.
     */
    string getRoughnessMap()
    {
        return this->roughnessMap;
    }

    /**
     * Set the ambient occlusion map of this leaf.
     */
    void setAOMap(string aoMap)
    {
        this->isPBR = true;
        this->aoMap = aoMap;
    }

    /**
     * Get the ambient occlusion map of this leaf.
     */
    string getAOMap()
    {
        return this->aoMap;
    }

    /**
     * get if PBR is used. Temporary testing code.
     */
    bool getPBRBool()
    {
        return this->isPBR;
    }

    /**
     * Get the nameof the normal texture corresponding to this leaf.
     */
    string getNormalMap()
    {
        return normalMap;
    }

    /**
     * Get the texture transform matrix of this leaf.
     */
    glm::mat4 getTextureTransform()
    {
        return this->textureTransform;
    }

    /**
     * Set the texture transform matrix of this leaf.
     */
    void setTextureTransform(glm::mat4 texTransform)
    {
        this->textureTransform = texTransform;
    }

    /**
     * Get the name of the instance this leaf contains
     * 
     * @return string 
     */
    string getInstanceOf() {
        return this->objInstanceName;
    }

    /**
     * Get a copy of this node.
     * 
     * @return SGNode* 
     */

    SGNode *clone() {
        LeafNode *newclone = new LeafNode(this->objInstanceName,material,name,scenegraph, this->textureMap, this->normalMap);
        return newclone;
    }

    /**
     * Visit this node.
     * 
     */
    void accept(SGNodeVisitor* visitor) {
      visitor->visitLeafNode(this);
    }
};
}
#endif
