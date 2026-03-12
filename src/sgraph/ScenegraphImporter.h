#ifndef _SCENEGRAPHIMPORTER_H_
#define _SCENEGRAPHIMPORTER_H_

#include "IScenegraph.h"
#include "Scenegraph.h"
#include "GroupNode.h"
#include "LeafNode.h"
#include "TransformNode.h"
#include "RotateTransform.h"
#include "ScaleTransform.h"
#include "TranslateTransform.h"
#include "DynamicTransform.h"
#include "SRTNode.h"
#include "PPMImageLoader.h"
#include "STBImageLoader.h"
#include "PolygonMesh.h"
#include "Material.h"
#include "Light.h"
#include "TextureImage.h"
#include "ObjAdjImporter.h"
#include <istream>
#include <fstream>
#include <map>
#include <string>
#include <iostream>
using namespace std;

namespace sgraph {
    class ScenegraphImporter {
        public:
            ScenegraphImporter() 
            {
            }

            IScenegraph *parse(istream& input) {
                string command;
                string inputWithOutCommentsString = stripComments(input);
                istringstream inputWithOutComments(inputWithOutCommentsString);
                IScenegraph *scenegraph = new Scenegraph();
                while (inputWithOutComments >> command) {
                    cout << "Read " << command << endl;
                    if (command == "instance") {
                        string name,path;
                        inputWithOutComments >> name >> path;
                        cout << "Read " << name << " " << path << endl;
                        meshPaths[name] = path;
                        ifstream in(path);
                       if (in.is_open()) {
                        util::PolygonMesh<VertexAttrib> mesh = util::ObjAdjImporter<VertexAttrib>::importFile(in,false);
                        meshes[name] = mesh;         
                       } 
                    }
                    else if (command == "light")
                    {
                        parseLight(inputWithOutComments);
                    }
                    else if (command == "assign-light")
                    {
                        parseAssignLight(inputWithOutComments);
                    }
                    else if (command == "dynamic")
                    {
                        parseDynamic(inputWithOutComments, scenegraph);
                    }
                    else if (command == "srt")
                    {
                        parseSRT(inputWithOutComments, scenegraph);
                    }
                    else if (command == "image")
                    {
                        parseTexture(inputWithOutComments);
                    }
                    else if (command == "assign-texture")
                    {
                        parseAssignTexture(inputWithOutComments);
                    }
                    else if (command == "assign-normal")
                    {
                        parseAssignNormal(inputWithOutComments);
                    }
                    else if (command == "assign-metallic")
                    {
                        parseAssignMetallic(inputWithOutComments);
                    }
                    else if (command == "assign-roughness")
                    {
                        parseAssignRoughness(inputWithOutComments);
                    }
                    else if (command == "assign-ao")
                    {
                        parseAssignAO(inputWithOutComments);
                    }
                    else if (command == "group") {
                        parseGroup(inputWithOutComments,scenegraph);
                    }
                    else if (command == "leaf") {
                        parseLeaf(inputWithOutComments, scenegraph);
                    }
                    else if (command == "material") {
                        parseMaterial(inputWithOutComments);
                    }
                    else if (command == "scale") {
                        parseScale(inputWithOutComments, scenegraph);
                    }
                    else if (command == "rotate") {
                        parseRotate(inputWithOutComments, scenegraph);
                    }
                    else if (command == "translate") {
                        parseTranslate(inputWithOutComments, scenegraph);
                    }
                    else if (command == "copy") {
                        parseCopy(inputWithOutComments);
                    }
                    else if (command == "import") {
                        parseImport(inputWithOutComments);
                    }
                    else if (command == "assign-material") {
                        parseAssignMaterial(inputWithOutComments);
                    }
                    else if (command == "add-child") {
                        parseAddChild(inputWithOutComments);
                    }
                    else if (command == "assign-root") {
                        parseSetRoot(inputWithOutComments);
                    }
                    else if (command == "cubemap") 
                    {
                        parseCubeMap(inputWithOutComments);
                    }
                    else {
                        throw runtime_error("Unrecognized or out-of-place command: "+command);
                    }
                }
                if (root!=NULL) {
                    scenegraph->makeScenegraph(root);
                    scenegraph->setMeshes(meshes);
                    scenegraph->setMeshPaths(meshPaths);
                    return scenegraph;
                }
                else {
                    throw runtime_error("Parsed scene graph, but nothing set as root");
                }
            }

            map<string, util::TextureImage*> getTextureMap()
            {
                return this->textureMap;
            }

            map<string, string> getTexturePaths()
            {
                return this->texturePaths;
            }


            map<string, SGNode*> getNodeMap()
            {
                return this->nodes;
            }

            vector<util::TextureImage*> getCubeMap()
            {
                return this->cubeMap;
            }

            vector<string> getCubeMapPaths()
            {
                return this->cubeMapPaths;
            }
            protected:

                virtual void parseTexture(istream& input)
                {
                    string texName, texPath;
                    input >> texName >> texPath;
                    cout << "Read " << texName << " " << texPath << endl;
                    ImageLoader* textureLoader;
                    if(texPath.find(".ppm") != string::npos)
                        textureLoader = new PPMImageLoader();
                    else
                        textureLoader = new STBImageLoader();
                    textureLoader->load(texPath);
                    util::TextureImage* texImage = new util::TextureImage(textureLoader->getPixels(), textureLoader->getWidth(), textureLoader->getHeight(), texName); // directly converting to reference. Hope this works.
                    textureMap[texName] = texImage;
                    texturePaths[texName] = texPath;
                }


                virtual void parseCubeMap(istream& input)
                {
                    vector<string> textures;
                    string line, word;
                    // gymnastics to get split words :(
                    getline(input, line);
                    stringstream linestream;

                    linestream << line;
                    while(linestream  >> word)
                        textures.push_back(word);

                    if(textures.size() == 1)
                    {
                        // hdr map
                        STBImageLoader* stbLoader  = new STBImageLoader(true); // flip for HDR.
                        stbLoader->load(textures[0]);
                        util::TextureImage* texImage = new util::TextureImage(stbLoader->getFPixels(), stbLoader->getWidth(), stbLoader->getHeight(), "skybox-hdr");
                        cubeMap.push_back(texImage);
                        cubeMapPaths.push_back(textures[0]);
                    }
                    else
                    {
                        string names[] = {
                            "skybox-right",
                            "skybox-left",
                            "skybox-top",
                            "skybox-bottom",
                            "skybox-front",
                            "skybox-back",
                        }; // this is the exact order that must be followed.
                        cout << "Read cubemap" << endl;
                        PPMImageLoader* ppmLoader = new PPMImageLoader();
                        STBImageLoader* stbLoader = new STBImageLoader(false);
                        GLubyte* pixels;
                        int width, height;
                        for(int i = 0; i < 6; i++)
                        {
                            string texture = textures[i];
                            if(texture.find(".ppm") != string::npos)
                            {
                                ppmLoader->load(texture);
                                pixels = ppmLoader->getPixels();
                                width = ppmLoader->getWidth();
                                height = ppmLoader->getHeight();
                            }
                            else
                            {
                                stbLoader->load(texture);
                                pixels = stbLoader->getPixels();
                                width = stbLoader->getWidth();
                                height = stbLoader->getHeight();
                            }
                            util::TextureImage* texImage = new util::TextureImage(pixels, width, height, names[i]); 
                            cubeMap.push_back(texImage);
                            cubeMapPaths.push_back(texture);
                        }
                    }

                    cout<<"Finished reading cubemap textures!"<<endl;
                }

                virtual void parseAssignNormal(istream& input)
                {
                    string textureName, leafName;
                    input >> leafName >> textureName;

                    LeafNode *leafNode = dynamic_cast<LeafNode *>(nodes[leafName]);
                    if ((leafNode != nullptr) && (textureMap.find(textureName) != textureMap.end())) 
                        leafNode->setNormalMap(textureName);
                }

                virtual void parseAssignMetallic(istream& input)
                {
                    string textureName, leafName;
                    input >> leafName >> textureName;

                    LeafNode *leafNode = dynamic_cast<LeafNode *>(nodes[leafName]);
                    if ((leafNode != nullptr) && (textureMap.find(textureName) != textureMap.end())) 
                        leafNode->setMetallicMap(textureName);
                }
                virtual void parseAssignRoughness(istream& input)
                {
                    string textureName, leafName;
                    input >> leafName >> textureName;

                    LeafNode *leafNode = dynamic_cast<LeafNode *>(nodes[leafName]);
                    if ((leafNode != nullptr) && (textureMap.find(textureName) != textureMap.end())) 
                        leafNode->setRoughnessMap(textureName);
                }
                virtual void parseAssignAO(istream& input)
                {
                    string textureName, leafName;
                    input >> leafName >> textureName;

                    LeafNode *leafNode = dynamic_cast<LeafNode *>(nodes[leafName]);
                    if ((leafNode != nullptr) && (textureMap.find(textureName) != textureMap.end())) 
                        leafNode->setAOMap(textureName);
                }

                virtual void parseAssignTexture(istream& input)
                {
                    string textureName, leafName;
                    input >> leafName >> textureName;

                    LeafNode *leafNode = dynamic_cast<LeafNode *>(nodes[leafName]);
                    if ((leafNode != nullptr) && (textureMap.find(textureName) != textureMap.end())) 
                        leafNode->setTextureMap(textureName);
                }

                virtual void parseDynamic(istream& input, IScenegraph *scenegraph)
                {
                    string varname, name;
                    input >> varname >> name;
                    cout << "Read " << varname << " " << name << endl;
                    SGNode *dynamic = new DynamicTransform(glm::mat4(1.0), name, scenegraph);
                    nodes[varname] = dynamic;
                }

                virtual void parseSRT(istream& input, IScenegraph *scenegraph)
                {
                    string varname, name;
                    float sx, sy, sz, rx, ry, rz, tx, ty, tz;
                    input >> varname >> name;
                    cout << "Read " << varname << " " << name << endl;
                    input >> sx >> sy >> sz >> rx >> ry >> rz >> tx >> ty >> tz;
                    SGNode *srtNode = new SRTNode(tx, ty, tz, glm::radians(rx), glm::radians(ry), glm::radians(rz), sx, sy, sz, name, scenegraph);
                    nodes[varname] = srtNode;
                }

                virtual void parseGroup(istream& input, IScenegraph *scenegraph) {
                    string varname,name;
                    input >> varname >> name;
                    
                    cout << "Read " << varname << " " << name << endl;
                    SGNode *group = new GroupNode(name,scenegraph);
                    nodes[varname] = group;
                }

                virtual void parseLeaf(istream& input, IScenegraph *scenegraph) {
                    string varname,name,command,instanceof;
                    input >> varname >> name;
                    cout << "Read " << varname << " " << name << endl;
                    input >> command;
                    if (command == "instanceof") {
                        input >> instanceof;
                    }
                    SGNode *leaf = new LeafNode(instanceof,name,scenegraph); 
                    LeafNode* leafInstance = dynamic_cast<LeafNode*>(leaf);
                    nodes[varname] = leaf;
                } 

                virtual void parseScale(istream& input, IScenegraph *scenegraph) {
                    string varname,name;
                    input >> varname >> name;
                    float sx,sy,sz;
                    input >> sx >> sy >> sz;
                    SGNode *scaleNode = new ScaleTransform(sx,sy,sz,name,scenegraph);
                    nodes[varname] = scaleNode;
                }

                virtual void parseTranslate(istream& input, IScenegraph *scenegraph) {
                    string varname,name;
                    input >> varname >> name;
                    float tx,ty,tz;
                    input >> tx >> ty >> tz;
                    SGNode *translateNode = new TranslateTransform(tx,ty,tz,name,scenegraph);
                    nodes[varname] = translateNode;         
                }

                virtual void parseRotate(istream& input, IScenegraph *scenegraph) {
                    string varname,name;
                    input >> varname >> name;
                    float angleInDegrees,ax,ay,az;
                    input >> angleInDegrees >> ax >> ay >> az;
                    SGNode *rotateNode = new RotateTransform(glm::radians(angleInDegrees),ax,ay,az,name,scenegraph);
                    nodes[varname] = rotateNode;         
                }

                virtual void parseMaterial(istream& input) {
                    util::Material mat;
                    float r,g,b;
                    string name;
                    input >> name;
                    string command;
                    input >> command;
                    while (command!="end-material") {
                        if (command == "ambient") {
                            input >> r >> g >> b;
                            mat.setAmbient(r,g,b);
                        }
                        else if (command == "diffuse") {
                            input >> r >> g >> b;
                            mat.setDiffuse(r,g,b);
                        }
                        else if (command == "specular") {
                            input >> r >> g >> b;
                            mat.setSpecular(r,g,b);
                        }
                        else if (command == "emission") {
                            input >> r >> g >> b;
                            mat.setEmission(r,g,b);
                        }
                        else if (command == "shininess") {
                            input >> r;
                            mat.setShininess(r);
                        }
                        // setting up PBR stuff here!
                        else if (command == "albedo") {
                            input >> r >> g >> b;
                            mat.setAlbedo(r,g,b);
                        }
                        else if (command == "metallic") {
                            input >> r;
                            mat.setMetallic(r);
                        }
                        else if (command == "roughness") {
                            input >> r;
                            mat.setRoughness(r);
                        }
                        else if (command == "ao") {
                            input >> r;
                            mat.setAO(r);
                        }
                        else
                            throw runtime_error("Material property is not recognized : " + command);
                        input >> command;
                    }
                    materials[name] = mat;
                }

                virtual void parseLight(istream& input)
                {
                    util::Light light;
                    float r, g, b;
                    float x, y, z;
                    string name;
                    input >> name;
                    light.setName(name);
                    string command;
                    input >> command;
                    while(command != "end-light")
                    {
                        if (command == "ambient")
                        {
                            input >> r >> g >> b;
                            light.setAmbient(r, g, b);
                        }
                        else if (command == "diffuse")
                        {
                            input >> r >> g >> b;
                            light.setDiffuse(r, g, b);
                        }
                        else if (command == "specular")
                        {
                            input >> r >> g >> b;
                            light.setSpecular(r, g, b);
                        }
                        else if (command == "position")
                        {
                            input >> x >> y >> z;
                            light.setPosition(x, y, z);
                        }
                        else if (command == "spot-direction")
                        {
                            input >> x >> y >> z;
                            light.setSpotDirection(x, y, z);
                        }
                        else if (command == "spot-angle")
                        {
                            input >> x;
                            light.setSpotAngle(x);
                        }
                        // PBR color here
                        else if (command == "color")
                        {
                            input >> r >> g >> b;
                            light.setColor(r, g, b);
                        }
                        else
                            throw runtime_error("Light property is not recognized : " + command);
                        input >> command;
                    }
                    lights[name] = light;
                }

                virtual void parseAssignLight(istream& input)
                {
                    string lightName, parentName;
                    input >> lightName >> parentName;

                    SGNode* node = nodes[parentName]; 
                    if ((node != nullptr) && (lights.find(lightName) != lights.end())) 
                    {
                        node->addLight(lights[lightName]);
                    }

                }

                virtual void parseCopy(istream& input) {
                    string nodename,copyof;

                    input >> nodename >> copyof;
                    if (nodes.find(copyof)!=nodes.end()) {
                        SGNode * copy = nodes[copyof]->clone();
                        nodes[nodename] = copy;
                    }
                }

                virtual void parseImport(istream& input) {
                    string nodename,filepath;

                    input >> nodename >> filepath;
                    ifstream external_scenegraph_file(filepath);
                    if (external_scenegraph_file.is_open()) {
                        
                        IScenegraph *importedSG = parse(external_scenegraph_file);
                        nodes[nodename] = importedSG->getRoot();
                       /* for (map<string,util::PolygonMesh<VertexAttrib> >::iterator it=importedSG->getMeshes().begin();it!=importedSG->getMeshes().end();it++) {
                            this->meshes[it->first] = it->second;
                        }
                        for (map<string,string>::iterator it=importedSG->getMeshPaths().begin();it!=importedSG->getMeshPaths().end();it++) {
                            this->meshPaths[it->first] = it->second;
                        }
                        */
                        //delete the imported scene graph but not its nodes!
                        importedSG->makeScenegraph(NULL);
                        delete importedSG;
                    }
                }

                virtual void parseAssignMaterial(istream& input) {
                    string nodename,matname;
                    input >> nodename >> matname;

                    LeafNode *leafNode = dynamic_cast<LeafNode *>(nodes[nodename]);
                    if ((leafNode!=NULL) && (materials.find(matname)!=materials.end())) {
                        leafNode->setMaterial(materials[matname]);
                    }
                }

                virtual void parseAddChild(istream& input) {
                    string childname,parentname;

                    input >> childname >> parentname;
                    ParentSGNode * parentNode = dynamic_cast<ParentSGNode *>(nodes[parentname]);
                    SGNode * childNode = NULL;
                    if (nodes.find(childname)!=nodes.end()) {
                        childNode = nodes[childname];
                    }

                    if ((parentNode!=NULL) && (childNode!=NULL)) {
                        parentNode->addChild(childNode);
                    }
                }

                virtual void parseSetRoot(istream& input) {
                    string rootname;
                    input >> rootname;

                    root = nodes[rootname];

                    cout << "Root's name is "<< root->getName() << endl;
                }

                string stripComments(istream& input) {
                    string line;
                    stringstream clean;
                    while (getline(input,line)) {
                        int i=0;
                        while ((i<line.length()) && (line[i]!='#')) {
                            clean << line[i];
                            i++;
                        }
                        clean << endl;
                    }
                    return clean.str();
                }
            private:
                map<string,SGNode *> nodes;
                map<string,util::Material> materials;
                map<string,util::PolygonMesh<VertexAttrib> > meshes;
                map<string,string> meshPaths;
                map<string, string> texturePaths;
                SGNode *root;
                map<string, util::Light> lights;
                map<string,util::TextureImage*> textureMap;
                vector<util::TextureImage*> cubeMap; // need to keep this separate because GPU transfer is done differently.
                vector<string> cubeMapPaths;
                // map<string,util::TextureImage*> normalMap; // for bump mapping.
                //removed references to defaultTexturePath and defaultNormalPath because its not needed after the constructor :)
    };
}


#endif