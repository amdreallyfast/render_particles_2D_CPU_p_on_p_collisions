#include "ParticleStorage.h"

#include "glload/include/glload/gl_4_4.h"

/*-----------------------------------------------------------------------------------------------
Description:
    Ensures that the object starts object with initialized values.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
ParticleStorage::ParticleStorage() :
    _vaoId(0),
    _arrayBufferId(0),
    _drawStyle(0),
    _sizeBytes(0)
{
}

/*-----------------------------------------------------------------------------------------------
Description:
    Generates a vertex buffer and vertex array object (contains vertex array attributes) for the 
    provided particle data.
Parameters:
    programId       Program binding is required for vertex attributes.
    numParticles    New memory is allocated to fit this number of particles.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleStorage::Init(unsigned int programId, unsigned int numParticles)
{
    // take care of the easy stuff first
    _allParticles.resize(numParticles);
    _sizeBytes = sizeof(Particle) * numParticles;
    _drawStyle = GL_POINTS;

    // MUST bind the program beforehand or else the VAO generation and binding will blow up
    glUseProgram(programId);

    glGenVertexArrays(1, &_vaoId);
    glGenBuffers(1, &_arrayBufferId);

    // the order of vertex array / buffer array binding doesn't matter so long as both are bound 
    // before setting vertex array attributes
    glBindVertexArray(_vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, _arrayBufferId);

    // just allocate space now, and send updated data at render time
    GLuint bufferSizeBytes = sizeof(Particle);
    bufferSizeBytes *= numParticles;
    glBufferData(GL_ARRAY_BUFFER, bufferSizeBytes, 0, GL_DYNAMIC_DRAW);

    // position appears first in structure and so is attribute 0 
    // velocity appears second and is attribute 1
    // "is active" flag is third and is attribute 2
    unsigned int vertexArrayIndex = 0; 
    unsigned int bufferStartOffset = 0;

    unsigned int bytesPerStep = sizeof(Particle);

    // position
    GLenum itemType = GL_FLOAT;
    unsigned int numItems = sizeof(Particle::_position) / sizeof(float);
    glEnableVertexAttribArray(vertexArrayIndex);
    glVertexAttribPointer(vertexArrayIndex, numItems, itemType, GL_FALSE, bytesPerStep, (void *)bufferStartOffset);

    // velocity
    itemType = GL_FLOAT;
    numItems = sizeof(Particle::_velocity) / sizeof(float);
    bufferStartOffset += sizeof(Particle::_position);
    vertexArrayIndex++;
    glEnableVertexAttribArray(vertexArrayIndex);
    glVertexAttribPointer(vertexArrayIndex, numItems, itemType, GL_FALSE, bytesPerStep, (void *)bufferStartOffset);

    // ignoring the "is active" flag by not telling OpenGL that there is an item here 
    // Note: Does this waste bytes? Yes, but it would be more work to pluck out the position and 
    // velocity and make those contiguous in another data structure than it would be to simply 
    // ignore the "is active" flag.
    //// "is active" flag
    //itemType = GL_INT;
    //numItems = sizeof(Particle::_isActive) / sizeof(int);
    //bufferStartOffset += sizeof(Particle::_velocity);
    //vertexArrayIndex++;
    //glEnableVertexAttribArray(vertexArrayIndex);
    //glVertexAttribPointer(vertexArrayIndex, numItems, itemType, GL_FALSE, bytesPerStep, (void *)bufferStartOffset);

    // cleanup
    glBindVertexArray(0);   // unbind this BEFORE the array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);    // always last
}
