#ifndef _OBJADJIMPORTER_H_
#define _OBJADJIMPORTER_H_

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include "PolygonMesh.h"
using namespace std;

namespace util
{

    /*
     * A helper class to import a PolygonMesh object from an OBJ file.
     * It imports only position, normal and texture coordinate data (if present).
     * The only difference between this class and the normal ObjImporter class is
     * the primitive type is GL_TRIANGLES_ADJACENCY and there are a few other
     * helper methods and structs to incorporate this.
     */
    template <class K>
    class ObjAdjImporter
    {
    private:
        // Struct to represent a unique vertex combination
        struct VertexKey
        {
            int posIndex;
            int texIndex;
            int normalIndex;
            
            VertexKey(int pos = -1, int tex = -1, int norm = -1) 
                : posIndex(pos), texIndex(tex), normalIndex(norm) {}
            
            bool operator<(const VertexKey& other) const
            {
                if (posIndex != other.posIndex) return posIndex < other.posIndex;
                if (texIndex != other.texIndex) return texIndex < other.texIndex;
                return normalIndex < other.normalIndex;
            }
        };

        struct Edge
        {
            unsigned int vert1, vert2;
            Edge(unsigned int vertice1, unsigned int vertice2)
            {
                // vert1 is always smaller than vert2. Easy for comparision that way.
                vert1 = min(vertice1, vertice2);
                vert2 = max(vertice1, vertice2);
            }
            bool operator==(const Edge &other) const
            {
                return vert1 == other.vert1 && vert2 == other.vert2;
            }
            bool operator!=(const Edge &other) const
            {
                return vert1 != other.vert1 || vert2 != other.vert2;
            }
            // Random < overload to use in map.
            bool operator<(const Edge &other) const
            {
                if (vert1 != other.vert1)
                    return vert1 < other.vert1;
                return vert2 < other.vert2;
            }
        };

        struct Face
        {
            vector<unsigned int> vertices;
            unsigned int index;
            Face(unsigned int vert1, unsigned int vert2, unsigned int vert3, int index)
            {
                vertices.push_back(vert1);
                vertices.push_back(vert2);
                vertices.push_back(vert3);
                this->index = index;
            }
            // return the vertice opposite to the edge, or -1 otherwise.
            int findOppVertice(Edge edge) const
            {
                int edgeVert1 = edge.vert1, edgeVert2 = edge.vert2;

                for (int i = 0; i < vertices.size(); i++)
                {
                    if (vertices[i] != edgeVert1 && vertices[i] != edgeVert2)
                        return vertices[i];
                }
                return -1;
            }

            bool operator==(const Face &other) const
            {
                return this->index == other.index;
            }

            bool operator!=(const Face &other) const
            {
                return this->index != other.index;
            }
        };

        static vector<unsigned int> findAdjacencies(const vector<unsigned int> &triangles)
        {
            // cout << "finding adjacencies!" << endl;
            // create map between each edge and the two edges that share it!
            vector<unsigned int> indices;
            map<Edge, vector<Face>> edgeMap;

            for (int i = 0; i < triangles.size(); i += 3)
            {
                // i, i+1, i+2
                unsigned int vert1 = triangles[i], vert2 = triangles[i + 1], vert3 = triangles[i + 2];
                Face face(vert1, vert2, vert3, i / 3);
                Edge edge1(vert1, vert2), edge2(vert2, vert3), edge3(vert3, vert1);
                edgeMap[edge1].push_back(face);
                edgeMap[edge2].push_back(face);
                edgeMap[edge3].push_back(face);
            }
            // for each vertex, find the adjacent vertex, then push both!
            for (int i = 0; i < triangles.size(); i += 3)
            {
                int vert1 = triangles[i], vert2 = triangles[i + 1], vert3 = triangles[i + 2];
                Face face(vert1, vert2, vert3, i / 3);
                Edge edge1(vert1, vert2), edge2(vert2, vert3), edge3(vert3, vert1);
                vector<Edge> edges = {edge1, edge2, edge3};
                // each edge should have 2 faces.
                // the order is crucial! v0, a0, v1, a1, v2, a2
                int adjVert1 = vert3, adjVert2 = vert1, adjVert3 = vert2; // changed this to have fallbacks!

                vector<Face>& faces1 = edgeMap[edge1];
                if (faces1.size() > 1)
                { // Check if there's more than one face
                    for (const Face &f : faces1)
                    {
                        if (f != face)
                        {
                            adjVert1 = f.findOppVertice(edge1);
                            break;
                        }
                    }
                }

                // Process edge2 (v2-v3)
                vector<Face>& faces2 = edgeMap[edge2];
                if (faces2.size() > 1)
                {
                    for (const Face &f : faces2)
                    {
                        if (f != face)
                        {
                            adjVert2 = f.findOppVertice(edge2);
                            break;
                        }
                    }
                }

                // Process edge3 (v3-v1)
                vector<Face>& faces3 = edgeMap[edge3];
                if (faces3.size() > 1)
                {
                    for (const Face& f : faces3)
                    {
                        if (f != face)
                        {
                            adjVert3 = f.findOppVertice(edge3);
                            break;
                        }
                    }
                }
                indices.push_back(vert1);
                indices.push_back(adjVert1);
                indices.push_back(vert2);
                indices.push_back(adjVert2);
                indices.push_back(vert3);
                indices.push_back(adjVert3);
            }
            return indices;
        }

    public:
        static PolygonMesh<K> importFile(ifstream &in, bool scaleAndCenter)
        {
            vector<glm::vec4> vertices, normals, texcoords;
            vector<unsigned int> triangles, triangle_texture_indices, triangle_normal_indices;
            int i, j;
            int lineno;
            PolygonMesh<K> mesh;

            // Map to track unique vertex combinations
            map<VertexKey, unsigned int> vertexMap;
            vector<VertexKey> uniqueVertices;
            unsigned int nextVertexIndex = 0;

            lineno = 0;

            string line;

            while (getline(in, line))
            {
                lineno++;
                if ((line.length() <= 0) || (line[0] == '#'))
                {
                    // line is a comment, ignore
                    continue;
                }

                stringstream str;

                str << line;

                vector<string> tokens;
                string symbol;

                while (str >> symbol)
                    tokens.push_back(symbol);

                if (tokens.empty())
                    continue;

                if (tokens[0] == "v")
                {
                    if ((tokens.size() < 4) || (tokens.size() > 7))
                    {
                        str.str("");
                        str.clear();
                        str << "Line " << lineno << ": Vertex coordinate has an invalid number of values";
                        throw str.str();
                    }

                    float num;
                    glm::vec4 v;

                    str.str("");
                    str.clear();

                    str << tokens[1] << " " << tokens[2] << " " << tokens[3];

                    str >> v.x >> v.y >> v.z;
                    v.w = 1.0f;

                    if (tokens.size() == 5)
                    {
                        str.str("");
                        str.clear();
                        str << tokens[4];
                        str >> num;
                        if (num != 0)
                        {
                            v.x /= num;
                            v.y /= num;
                            v.z /= num;
                        }
                    }

                    vertices.push_back(v);
                }
                else if (tokens[0] == "vt")
                {
                    if ((tokens.size() < 3) || (tokens.size() > 4))
                    {
                        str.str("");
                        str.clear();
                        str << "Line " << lineno << ": Texture coordinate has an invalid number of values";
                        throw str.str();
                    }

                    glm::vec4 v;

                    float n;

                    str.str("");
                    str.clear();
                    str << tokens[1] << " " << tokens[2];

                    str >> v.x >> v.y;
                    v.z = 0.0f;
                    v.w = 1.0f;

                    if (tokens.size() > 3)
                    {
                        str.str("");
                        str.clear();
                        str << tokens[3];
                        str >> v.z;
                    }

                    texcoords.push_back(v);
                }
                else if (tokens[0] == "vn")
                {
                    if (tokens.size() != 4)
                    {
                        str.str("");
                        str.clear();
                        str << "Line " << lineno << ": Normal has an invalid number of values";
                        throw str.str();
                    }

                    float num;
                    glm::vec3 v;

                    str.str("");
                    str.clear();
                    str << tokens[1] << " " << tokens[2] << " " << tokens[3];
                    str >> v.x >> v.y >> v.z;

                    v = glm::normalize(v);
                    normals.push_back(glm::vec4(v, 0.0f));
                }
                else if (tokens[0] == "f")
                {
                    if (tokens.size() < 4)
                    {
                        str.str("");
                        str.clear();
                        str << "Line " << lineno << ": Face has too few vertices, must be at least 3";
                        throw str.str();
                    }

                    vector<unsigned int> faceVertices;
                    vector<unsigned int> t_triangles, t_tex, t_normal;

                    for (i = 1; i < tokens.size(); i++)
                    {
                        str.str("");
                        str.clear();
                        str << tokens[i];

                        vector<string> data;
                        string temp;

                        while (getline(str, temp, '/'))
                            data.push_back(temp);

                        if ((data.size() < 1) || (data.size() > 3))
                        {
                            str.str("");
                            str.clear();
                            str << "Line " << lineno << ": Face specification has an incorrect number of values";
                            throw str.str();
                        }

                        // Parse vertex indices (OBJ is 1-based, convert to 0-based)
                        VertexKey key;
                        // in OBJ file format all indices begin at 1, so must subtract 1 here
                        // Position index (required)
                        int vi;
                        str.str("");
                        str.clear();
                        str << data[0];
                        str >> vi;
                        t_triangles.push_back(vi - 1); // vertex index
                        key.posIndex = vi - 1;
                        
                        // Texture index (optional)
                        if (data.size() > 1 && data[1].length() > 0)
                        {
                            str.str("");
                            str.clear();
                            str << data[1];
                            str >> vi;
                            t_tex.push_back(vi - 1);
                            key.texIndex = vi - 1;
                            
                        }
                        
                        // Normal index (optional)
                        if (data.size() > 2 && data[2].length() > 0)
                        {
                            str.str("");
                            str.clear();
                            str << data[2];
                            str >> vi;
                            t_normal.push_back(vi - 1);
                            key.normalIndex = vi - 1;
                        }

                        // Check if this vertex combination already exists
                        if (vertexMap.find(key) == vertexMap.end())
                        {
                            // New unique vertex combination
                            vertexMap[key] = nextVertexIndex;
                            uniqueVertices.push_back(key);
                            nextVertexIndex++;
                        }
                        
                        faceVertices.push_back(vertexMap[key]);
                    }

                    if (faceVertices.size() < 3)
                    {
                        str.str("");
                        str.clear();
                        str << "Line " << lineno << ": Fewer than 3 vertices for a polygon";
                        throw str.str();
                    }

                    // Triangle fan for faces with more than 3 vertices
                    for (i = 2; i < faceVertices.size(); i++)
                    {
                        triangles.push_back(faceVertices[0]);
                        triangles.push_back(faceVertices[i - 1]);
                        triangles.push_back(faceVertices[i]);
                        
                        // if (t_tex.size() > 0)
                        // {
                        //     triangle_texture_indices.push_back(t_tex[0]);
                        //     triangle_texture_indices.push_back(t_tex[i - 1]);
                        //     triangle_texture_indices.push_back(t_tex[i]);
                        // }
    
                        // if (t_normal.size() > 0)
                        // {
                        //     triangle_normal_indices.push_back(t_normal[0]);
                        //     triangle_normal_indices.push_back(t_normal[i - 1]);
                        //     triangle_normal_indices.push_back(t_normal[i]);
                        // }
                    }
                }
            }

            // Scale and center if requested
            if (scaleAndCenter)
            {
                // center about the origin and within a cube of side 1 centered at the origin
                // find the centroid
                glm::vec4 center = vertices[0];

                glm::vec4 minimum = center;
                glm::vec4 maximum = center;

                for (i = 1; i < vertices.size(); i++)
                {
                    // center = center.add(vertices.get(i).x,vertices.get(i).y,vertices.get(i).z,0.0f);
                    minimum = glm::min(minimum, vertices[i]);
                    maximum = glm::max(maximum, vertices[i]);
                }

                center = (minimum + maximum) * 0.5f;

                float longest;

                longest = std::max(maximum.x - minimum.x, std::max(maximum.y - minimum.y, maximum.z - minimum.z));

                // first translate and then scale
                glm::mat4 transformMatrix = glm::scale(glm::mat4(1.0),
                                                       glm::vec3(1.0f / longest,
                                                                 1.0f / longest,
                                                                 1.0f / longest)) *
                                            glm::translate(glm::mat4(1.0),
                                                           glm::vec3(-center.x,
                                                                     -center.y,
                                                                     -center.z));

                // scale down each other
                for (i = 0; i < vertices.size(); i++)
                {
                    vertices[i] = transformMatrix * vertices[i];
                }
            }

            // Build final vertex data using unique combinations
            vector<K> vertexData;
            vector<float> data;
            // for (i = 0; i < vertices.size(); i++)
            // {
            //     K v;

            //     data.clear();
            //     data.push_back(vertices[i].x);
            //     data.push_back(vertices[i].y);
            //     data.push_back(vertices[i].z);
            //     data.push_back(vertices[i].w);

            //     v.setData("position", data);
            //     if (texcoords.size() == vertices.size())
            //     {
            //         data.clear();
            //         data.push_back(texcoords[i].x);
            //         data.push_back(texcoords[i].y);
            //         data.push_back(texcoords[i].z);
            //         data.push_back(texcoords[i].w);
            //         v.setData("texcoord", data);
            //     }
            //     if (normals.size() == vertices.size())
            //     {
            //         data.clear();
            //         data.push_back(normals[i].x);
            //         data.push_back(normals[i].y);
            //         data.push_back(normals[i].z);
            //         data.push_back(normals[i].w);
            //         v.setData("normal", data);
            //     }

            //     vertexData.push_back(v);
            // }

            for (const VertexKey& key : uniqueVertices)
            {
                K v;

                // Set position data
                if (key.posIndex >= 0 && key.posIndex < vertices.size())
                {
                    data.clear();
                    data.push_back(vertices[key.posIndex].x);
                    data.push_back(vertices[key.posIndex].y);
                    data.push_back(vertices[key.posIndex].z);
                    data.push_back(vertices[key.posIndex].w);
                    v.setData("position", data);
                }

                // Set texture coordinate data
                if (key.texIndex >= 0 && key.texIndex < texcoords.size())
                {
                    data.clear();
                    data.push_back(texcoords[key.texIndex].x);
                    data.push_back(texcoords[key.texIndex].y);
                    data.push_back(texcoords[key.texIndex].z);
                    data.push_back(texcoords[key.texIndex].w);
                    v.setData("texcoord", data);
                }

                // Set normal data
                if (key.normalIndex >= 0 && key.normalIndex < normals.size())
                {
                    data.clear();
                    data.push_back(normals[key.normalIndex].x);
                    data.push_back(normals[key.normalIndex].y);
                    data.push_back(normals[key.normalIndex].z);
                    data.push_back(normals[key.normalIndex].w);
                    v.setData("normal", data);
                }

                vertexData.push_back(v);
            }

            // Compute normals if not provided
            if (normals.size() == 0)
                mesh.computeNormals();

            mesh.setVertexData(vertexData);
            mesh.setPrimitives(findAdjacencies(triangles));
            mesh.setPrimitiveType(GL_TRIANGLES_ADJACENCY);
            mesh.setPrimitiveSize(6);
            return mesh;
        }
    };

}

#endif
