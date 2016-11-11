#pragma once

#include "Particle.h"
#include <vector>

/*-----------------------------------------------------------------------------------------------
Description:
    Associates a particle collection with an OpenGL buffer and any necessary vertex attributes.

    This is a struct because I can't think of a good reason to make it private.  The OpenGL data 
    should definitely be restricted, but the particle collection needs to be accessible by the 
    ParticleUpdater class, so I could jump through some hoops to make it accessible through a 
    friend interface or something, or I could just make them public and be nice and not mess 
    them up.
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
struct ParticleStorage
{
public:
    ParticleStorage();
    void Init(unsigned int programId, unsigned int numParticles);

    // save on the large header inclusion of OpenGL and write out these primitive types instead 
    // of using the OpenGL typedefs
    // Note: IDs are GLuint (unsigned int), draw style is GLenum (unsigned int), GLushort is 
    // unsigned short.
    unsigned int _vaoId;
    unsigned int _arrayBufferId;
    unsigned int _drawStyle;    // GL_TRIANGLES, GL_LINES, etc.
    unsigned int _sizeBytes;    // useful for glBufferSubData(...)
    std::vector<Particle> _allParticles;
};

