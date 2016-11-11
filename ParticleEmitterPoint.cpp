#include "ParticleEmitterPoint.h"

#include "RandomToast.h"
#include "glm/detail/func_geometric.hpp" // for normalizing glm vectors

/*-----------------------------------------------------------------------------------------------
Description:
    Ensures that the object starts object with initialized values.
Parameters:
    emitterPos  A 2D vector in window space (XY on range [-1,+1]).
    minVel      The minimum velocity for particles being emitted.
    maxVel      The maximum emission velocity.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
ParticleEmitterPoint::ParticleEmitterPoint(const glm::vec2 &emitterPos, const float minVel, const float maxVel)
{
    _originalPosition = emitterPos;
    _currentPosition = emitterPos;
    _velocityCalculator.SetMinMaxVelocity(minVel, maxVel);
    _velocityCalculator.UseRandomDir();
}

/*-----------------------------------------------------------------------------------------------
Description:
    Sets the given particle's starting position and velocity.  Does NOT alter the "is active" 
    flag.  That flag is altered only by the "particle updater" object.
Parameters:
    resetThis   Self-explanatory.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleEmitterPoint::ResetParticle(Particle *resetThis) const
{
    // give it some flavor by making the particles be reset to within a range near the emitter's 
    // position, making it look like a particle hotspot
    // Note: Making the particle reset in a random position in a small radius from the emitter 
    // center is more involved than I initially thought.  
    // - Random direction, get a random X and a random Y and then normalize that vector.  
    // - Random distance along that direction vector, get a random number between 0 and 1.  
    // - Shrink it to the desired size with a scalar.
    float xOffset = (float)(RandomPosAndNeg() % 100);
    float yOffset = (float)(RandomPosAndNeg() % 100);
    glm::vec2 offset = 0.05f * RandomOnRange0to1() * glm::normalize(glm::vec2(xOffset, yOffset));
    resetThis->_position = _currentPosition + offset;

    resetThis->_velocity = _velocityCalculator.GetNew();
}

/*-----------------------------------------------------------------------------------------------
Description:
    Applies the transform to the emission point.
Parameters:
    m       A 4x4 transform matrix.  Because glm transform functions only spit out a 4x4.
Returns:    None
Exception:  Safe
Creator:    John Cox (8-26-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleEmitterPoint::SetTransform(const glm::mat4 &m)
{
    _currentPosition = glm::vec2(m * glm::vec4(_originalPosition, 0.0f, 1.0f));
}
