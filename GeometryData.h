#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include <vector>

/*-----------------------------------------------------------------------------------------------
Description:
    Stores all info necessary to draw a single vertex.
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
struct MyVertex
{
    glm::vec2 _position;
    glm::vec2 _texturePosition;
};

/*-----------------------------------------------------------------------------------------------
Description:
    Stores all info necessary to draw a chunk of vertices and access the info later if 
    neccessary.
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
struct GeometryData
{
    GeometryData();
    void Init(unsigned int programId);


    // save on the large header inclusion of OpenGL and write out these primitive types instead 
    // of using the OpenGL typedefs
    // Note: IDs are GLuint (unsigned int), draw style is GLenum (unsigned int), GLushort is 
    // unsigned short.
    unsigned int _vaoId;
    unsigned int _arrayBufferId;
    unsigned int _elementBufferId;
    unsigned int _drawStyle;  // GL_TRIANGLES, GL_LINES, etc.
    std::vector<MyVertex> _verts;
    std::vector<unsigned short> _indices;
};