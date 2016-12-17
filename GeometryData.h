#pragma once

#include <vector>
#include "MyVertex.h"

/*-----------------------------------------------------------------------------------------------
Description:
    Stores all info necessary to draw a chunk of vertices and access the info later if 
    neccessary.
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
struct GeometryData
{
    GeometryData();
    ~GeometryData();
    void Init(unsigned int programId);
    void UpdateBufferData();


    // save on the large header inclusion of OpenGL and write out these primitive types instead 
    // of using the OpenGL typedefs
    // Note: IDs are GLuint (unsigned int), draw style is GLenum (unsigned int), GLushort is 
    // unsigned short.
    unsigned int _vaoId;
    unsigned int _arrayBufferId;
    unsigned int _elementBufferId;
    unsigned int _drawStyle;    // GL_TRIANGLES, GL_LINES, etc.
    unsigned int _vertexBufferSizeBytes;
    unsigned int _elementBufferSizeBytes;
    std::vector<MyVertex> _verts;
    std::vector<unsigned short> _indices;
};