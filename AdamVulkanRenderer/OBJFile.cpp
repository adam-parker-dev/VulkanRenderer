#include "stdafx.h"
#include "OBJFile.h"
#include "Vec3.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <map>

void OBJFile::LoadFile(std::string fileName, Model &model)
{
    // Store the raw vertices, normals, uvs per face
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec3> uvs;

    // For each face, store indices to vertex
    // 0 1 2 3 is a square face
    // becomes two triangles, 0 1 2 and 0 2 3.
    // formula for triangles is 0 (i) (i+1) [for i in 1..(n-2)]
    // Vertices are stored in a counter-clockwise order by default
    // https://en.wikipedia.org/wiki/Wavefront_.obj_file
    // http://stackoverflow.com/questions/23723993/converting-quadriladerals-in-an-obj-file-into-triangles
    std::vector<int> vertexIndices;
    std::vector<int> normalsIndices;
    std::vector<int> uvIndices;
    std::map<std::pair<int, int>, int> vertexNormalPair;    

    std::ifstream objFile(fileName.c_str());
    std::string fileData;
    std::string line;

    if (objFile.is_open())
    {
        while (std::getline(objFile, line))
        {
            // remove trailing return character and white space
			if (line.length() <= 0)
			{
				continue;
			}
            string::iterator lastChar = line.end() - 1;
            while (*lastChar == ' ' || *lastChar == '\r')
            {
                line.pop_back();
                lastChar = line.end() - 1;
            }

            // ******************************************************************
            // TODO: Implement for UV coordinates when we start importing textures! 
            if (line.find("vt") != std::string::npos)
            {

                /*                size_t position = line.find(' ');
                                while (position != std::string::npos)
                                {
                                std::string token = line.substr(position, position = line.find(' '));
                                uvs.push_back(static_cast<float>(atof(token.c_str())));
                                }*/
            }
            // Import the normals
            // TODO: Fixing floating point imprecision from double->float convert
            else if (line.find("vn") != std::string::npos)
            {
                Vec3 entry;
                int index = 0;
                size_t position = line.find(' ');
                line = line.substr(position + 1, line.size() - position);
                while (position != std::string::npos)
                {           
                    position = line.find(' ');
                    std::string token = line.substr(0, position);
                    if (!token.empty())
                    {                        
                        const char* tokenString = token.c_str();
                        entry[index] = std::strtod(tokenString, 0);
                        //model.fileNormals.push_back(entry[index]);
                        ++index;
                    }
                    line = line.substr(position + 1, line.size() - position);
                }
                normals.push_back(entry);
            }
            // Import the vertices
            // TODO: Fixing floating point imprecision from double->float convert
            else if (line.find("v ") != std::string::npos)
            {
                Vec3 entry;
                int index = 0;
                size_t position = line.find(' ');
                line = line.substr(position + 1, line.size() - position);
                while (position != std::string::npos)
                {           
                    position = line.find(' ');
                    std::string token = line.substr(0, position);                    
                    if (!token.empty())
                    {                        
                        const char* tokenString = token.c_str();
                        entry[index] = std::strtod(tokenString, 0);
                        //model.fileVertices.push_back(entry[index]);                       
                        ++index;
                    }
                    line = line.substr(position + 1, line.size() - position);
                }
                vertices.push_back(entry);
            }

            // Import the faces via indices for vertices, normals, and uvs
            // TODO: Fixing floating point imprecision from double->float convert
            else if (line.find("f ") != std::string::npos)
            {               
                // find the first entry
                size_t position = line.find(' ');
                line = line.substr(position + 1, line.size() - position);

                // Store variables for vertex tracking and triangle building
                int faceVert = 0;
                int firstFaceVert = 0;   
                bool lastLine = false;

                // Process the line
                while (position != std::string::npos)
                {
                    position = line.find(' ');                    

                    std::string token = line.substr(0, position);
                    line = line.substr(position + 1, line.size() - position);

                    if (!token.empty()) // hack to get rid of trailing space
                    {
                        size_t slashPosition = token.find('/');

                        // Vertex index
                        int vindex = atoi(token.substr(0, slashPosition).c_str()) -1; // indices are 1 based and should be 0 based                    

                        // UV index
                        token = token.substr(slashPosition + 1, token.size() - slashPosition);
                        slashPosition = token.find('/') ;
                        int uvindex = atoi(token.substr(0, slashPosition).c_str()) - 1;
                        uvIndices.push_back(uvindex);

                        // Normal index
                        token = token.substr(slashPosition + 1, token.size() - slashPosition);
                        int nindex = atoi(token.c_str()) - 1;
                        normalsIndices.push_back(nindex);

                        // TODO: Implement smooth groups import.

                        // If we have normals, store vertex/normal pairs
                        if (!normals.empty())
                        {          
                            // Track vertex/normal pairs, only introduce new vertices when a new vertex/normal pair shows up.
                            // Map vertex/normal pair to an index for index buffer
                            std::map<std::pair<int, int>, int>::iterator vertNorm = vertexNormalPair.find(std::pair<int, int>(vindex, nindex));
                            if (vertNorm == vertexNormalPair.end())
                            {
                                for (vertNorm = vertexNormalPair.begin(); vertNorm != vertexNormalPair.end(); ++vertNorm)
                                {
                                    // A vertex already exists, but the normal pair for it does not
                                    // Create a new vertex/normal pair
                                    if (vindex == vertNorm->first.first)
                                    {
                                        vertexNormalPair.insert(std::pair<std::pair<int, int>, int>(std::pair<int, int>(vindex, nindex), (model.fileVertices.size() + 1) / 3));
                                        vertNorm = vertexNormalPair.find(std::pair<int, int>(vindex, nindex));
                                        model.fileVertices.push_back(vertices[vindex][0]);
                                        model.fileVertices.push_back(vertices[vindex][1]);
                                        model.fileVertices.push_back(vertices[vindex][2]);
                                        model.fileNormals.push_back(normals[nindex][0]);
                                        model.fileNormals.push_back(normals[nindex][1]);
                                        model.fileNormals.push_back(normals[nindex][2]);
                                        break;
                                    }
                                }
                                // No vertex has been stored, so create a new one
                                if (vertNorm == vertexNormalPair.end())
                                {
                                    vertexNormalPair.insert(std::pair<std::pair<int, int>, int>(std::pair<int, int>(vindex, nindex), (model.fileVertices.size() + 1) / 3));
                                    vertNorm = vertexNormalPair.find(std::pair<int, int>(vindex, nindex));
                                    model.fileVertices.push_back(vertices[vindex][0]);
                                    model.fileVertices.push_back(vertices[vindex][1]);
                                    model.fileVertices.push_back(vertices[vindex][2]);

                                    model.fileNormals.push_back(normals[nindex][0]);
                                    model.fileNormals.push_back(normals[nindex][1]);
                                    model.fileNormals.push_back(normals[nindex][2]);
                                }
                            }

                            // Push back the index buffer entry
                            model.fileIndices.push_back(vertNorm->second);

                            // Store if this is the first face vertex
                            if (faceVert == 0)
                            {
                                firstFaceVert = vertNorm->second;
                            }
                            ++faceVert;

                            // If we are beyond the third vertex, it is time to start a new triangle
                            if (faceVert >= 3 && !lastLine)
                            {
                                model.fileIndices.push_back(firstFaceVert);
                                model.fileIndices.push_back(vertNorm->second);
                            }

                            // Determine if we are on the last line, so we don't start a new triangle next loop
                            if (line.find(' ') == std::string::npos)
                            {
                                lastLine = true;
                            }
                        }
                        else
                        {
                            // No normals are present, so just push back the index
                            model.fileIndices.push_back(vindex);
                            if (faceVert == 0)
                            {
                                firstFaceVert = vindex;
                            }
                            ++faceVert;
                            if (faceVert >= 3 && !lastLine)
                            {
                                model.fileIndices.push_back(firstFaceVert);
                                model.fileIndices.push_back(vindex);
                            }

                            if (line.find(' ') == std::string::npos)
                            {
                                lastLine = true;
                            }
                        }

                        vertexIndices.push_back(vindex);
                    }
                }
            }
           
        }
        objFile.close();
    }

    // TODO: Cleanup so we don't have to create a normals buffer when no normals present
    // Requires some sort of flag in the render pipeline
    if (normals.empty())
    {
        for (int i = 0; i < vertices.size(); ++i)
        {
            model.fileVertices.push_back(vertices[i][0]);
            model.fileVertices.push_back(vertices[i][1]);
            model.fileVertices.push_back(vertices[i][2]);
            model.fileNormals.push_back(0);
            model.fileNormals.push_back(0);
            model.fileNormals.push_back(0);
        }
    }

    // Need to do find replace \r to \n for windows->android(linux)
    size_t pos;
    while ((pos = fileData.find('\r')) != std::string::npos)
    {
        fileData.replace(pos, std::string("\r").length(), "\n");
    }            
}
