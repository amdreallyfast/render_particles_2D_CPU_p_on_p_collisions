#pragma once

#include "glm/vec2.hpp"

/*-----------------------------------------------------------------------------------------------
Description:
    This is a simple structure that says where a particle is, where it is going, and whether it
    has gone out of bounds ("is active" flag).  That flag also serves to prevent all particles
    from going out all at once upon creation by letting the "particle updater" regulate how many
    are emitted every frame.
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
struct Particle
{
    Particle() :
        // glm structures already have "set to 0" constructors
        _mass(0.1f),
        _radiusOfInfluence(0.02f),
        _currentQuadTreeIndex(0),
        _isActive(0)
    {
    }

    glm::vec2 _position;
    glm::vec2 _velocity;

    // all particles are identical for now
    float _mass;

    // used for collision detection because a particle's position is float values, so two 
    // particles' positions are almost never going to be exactly equal
    float _radiusOfInfluence;

    // TODO: get rid of this
    int _currentQuadTreeIndex;

    // Note: Booleans cannot be uploaded to the shader 
    // (https://www.opengl.org/sdk/docs/man/html/glVertexAttribPointer.xhtml), so send the 
    // "is active" flag as an integer.  It is understood 
    int _isActive;
};