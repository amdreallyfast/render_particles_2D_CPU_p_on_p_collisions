#pragma once

#include "IParticleEmitter.h"
#include "Particle.h"
#include "MinMaxVelocity.h"
#include "glm/vec2.hpp"

/*-----------------------------------------------------------------------------------------------
Description:
    This particle emitter will reset particles to a position along a vector and will set their 
    velocity to the provided vector.  Particles will be launch in this direction evenly 
    distributed along the bar.
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
class ParticleEmitterBar : public IParticleEmitter
{
public:
    ParticleEmitterBar(const glm::vec2 &p1, const glm::vec2 &p2, const glm::vec2 &emitDir,
        const float minVel, const float maxVel);
    virtual void ResetParticle(Particle *resetThis) const;
    virtual void SetTransform(const glm::mat4 &m);
private:
    // I need the bar's start and start->end vector on every frame, but I don't need the end 
    // point except to calculate the start->end vector, so I'll calculate the later on class 
    // initialization and won't bother storing the end point
    glm::vec2 _originalBarStart;
    glm::vec2 _originalBarStartToEnd;
    glm::vec2 _currentBarStart;
    glm::vec2 _currentBarStartToEnd;

    glm::vec2 _originalEmitDirection;
    MinMaxVelocity _velocityCalculator;
};

