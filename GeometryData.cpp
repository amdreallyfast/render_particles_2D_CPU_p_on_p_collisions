#include "GeometryData.h"

#include "glload/include/glload/gl_4_4.h"

/*-----------------------------------------------------------------------------------------------
Description:
    Ensures that the structure starts object with initialized values.

    Note: OpenGL IDs can start at 0, but they are also unsigned, so they should not be set to 
    -1.  Just initialize them to 0 and be sure to not use one without being initialized.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
GeometryData::GeometryData() :
    _vaoId(0),
    _arrayBufferId(0),
    _elementBufferId(0)
{
}

/*-----------------------------------------------------------------------------------------------
Description:
    Ensures that class resources are released.  This should have been made months ago, but it
    wasn't.  
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
GeometryData::~GeometryData()
{
    glDeleteBuffers(1, &_arrayBufferId);
    glDeleteBuffers(1, &_elementBufferId);
    glDeleteVertexArrays(1, &_vaoId);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Generates a vertex buffer, index buffer, and vertex array object (contains vertex array
    attributes) for the provided geometry data.
Parameters:
    programId   Program binding is required for vertex attributes.
    initThis    Self-explanatory.
Returns:    None
Exception:  Safe
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
void GeometryData::Init(unsigned int programId)
{
    // must bind program or else the vertex arrays will either blow up or refer to a 
    // non-existent program
    glUseProgram(programId);

    // vertex array buffer
    glGenBuffers(1, &_arrayBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, _arrayBufferId);

    // number of bytes = number of items * size of said item
    _vertexBufferSizeBytes = _verts.size() * sizeof(_verts[0]);
    glBufferData(GL_ARRAY_BUFFER, _vertexBufferSizeBytes, _verts.data(), GL_STATIC_DRAW);

    // the order of vertex array / buffer array binding doesn't matter so long as both are bound 
    // before setting vertex array attributes
    glGenVertexArrays(1, &_vaoId);
    glBindVertexArray(_vaoId);

    unsigned int vertexAttribArrayIndex = 0;
    unsigned int vertexBufferStartOffset = 0;
    unsigned int strideSizeBytes = sizeof(MyVertex);

    // position
    // Note: Just hard code a 2 for the number of floats in a vec2.
    glEnableVertexAttribArray(vertexAttribArrayIndex);
    glVertexAttribPointer(vertexAttribArrayIndex, 2, GL_FLOAT, GL_FALSE,
        strideSizeBytes, (void *)vertexBufferStartOffset);

    // texture position
    // Note: Like for position, hard code the number of floats (2 for vec2).
    vertexAttribArrayIndex++;
    vertexBufferStartOffset += sizeof(MyVertex::_position);
    glEnableVertexAttribArray(vertexAttribArrayIndex);
    glVertexAttribPointer(vertexAttribArrayIndex, 2, GL_FLOAT, GL_FALSE,
        strideSizeBytes, (void *)vertexBufferStartOffset);

    // indices buffer
    glGenBuffers(1, &_elementBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _elementBufferId);

    _elementBufferSizeBytes = _indices.size() * sizeof(_indices[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _elementBufferSizeBytes, _indices.data(), GL_STATIC_DRAW);

    // must unbind array object BEFORE unbinding the buffer or else the array object will think 
    // that its vertex attribute pointers will believe that they should refer to buffer 0
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glUseProgram(0);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Uploads buffer data on the GPU with current vertex data.  Useful if the vertex data varies 
    in size on every frame, such as when rendering the quad tree.

    Node: Call after updating _verts or _indices
    Also Note: Will reset current bindings of GL_ARRAY_BUFFER and GL_ELEMENT_ARRAY_BUFFER to 0.
    Also Also Note: Does not need a bound program to use.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
void GeometryData::UpdateBufferData()
{
    glBindBuffer(GL_ARRAY_BUFFER, _arrayBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _elementBufferId);

    // upload new data, resizing if necessary
    unsigned int newVertexBufferSizeBytes = _verts.size() * sizeof(_verts[0]);
    if (newVertexBufferSizeBytes > _vertexBufferSizeBytes)
    {
        _vertexBufferSizeBytes = newVertexBufferSizeBytes;
        glBufferData(GL_ARRAY_BUFFER, _vertexBufferSizeBytes, _verts.data(), GL_STATIC_DRAW);
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, _vertexBufferSizeBytes, _verts.data());
    }

    // repeat for element buffer
    unsigned int newElementBuffersizeBytes = _indices.size() * sizeof(_indices[0]);
    if (newElementBuffersizeBytes > _elementBufferSizeBytes)
    {
        _elementBufferSizeBytes = newElementBuffersizeBytes;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _elementBufferSizeBytes, _indices.data(), GL_STATIC_DRAW);
    }
    else
    {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, _elementBufferSizeBytes, _indices.data());
    }

    // cleanup
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

