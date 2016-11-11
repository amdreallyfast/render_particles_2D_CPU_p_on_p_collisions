#include "ParticleEmitterBar.h"

#include "RandomToast.h"

/*-----------------------------------------------------------------------------------------------
Description:
    Ensures that the object starts object with initialized values.
Parameters:
    p1      The bar's origin point in window space (XY on range [-1,+1]).
    p2      The bar's end point.
    emitDir     Particles will be launched in this direction evenly along the bar.
    minVel      The minimum velocity for particles being emitted.
    maxVel      The maximum emission velocity.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
ParticleEmitterBar::ParticleEmitterBar(const glm::vec2 &p1, const glm::vec2 &p2, 
    const glm::vec2 &emitDir, float minVel, const float maxVel)
{
    _originalBarStart = p1;
    _originalBarStartToEnd = p2 - p1;
    _currentBarStart = _originalBarStart;
    _currentBarStartToEnd = _originalBarStartToEnd;
    _velocityCalculator.SetMinMaxVelocity(minVel, maxVel);

    //// rotate a 2D vector (x,y) by +90 degrees => (-y,x)
    //glm::vec2 plus90Degrees((-1.0f) * _barStartToEnd.y, _barStartToEnd.x);
    //_velocityCalculator.SetDir(plus90Degrees);
    _originalEmitDirection = emitDir;
    _velocityCalculator.SetDir(emitDir);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Sets the given particle's starting position and velocity.  The position is set to a random 
    point along the var.
    
    Does NOT alter the "is active" flag.  That flag is altered only by the "particle updater" 
    object.
Parameters:
resetThis   Self-explanatory.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleEmitterBar::ResetParticle(Particle *resetThis) const
{
    // give it some flavor by making the particles be reset to within a range near the emitter 
    // bar's position instead of exactly on the bar, making it look like a particle hotspot
    resetThis->_position = _currentBarStart + (RandomOnRange0to1() * _currentBarStartToEnd);

    resetThis->_velocity = _velocityCalculator.GetNew();
}

/*-----------------------------------------------------------------------------------------------
Description:
    Applies the transform to the emission direction and to the points that make up the bar.  The 
    emission direction is only rotated, while the bar points are rotated and translated.
Parameters:
    m       A 4x4 transform matrix.  Because glm transform functions only spit out a 4x4.
Returns:    None
Exception:  Safe
Creator:    John Cox (8-26-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleEmitterBar::SetTransform(const glm::mat4 &m)
{
    _currentBarStart = glm::vec2(m * glm::vec4(_originalBarStart, 0.0f, 1.0f));

    // "bar start to end" is relative to "bar start", so it can be rotated but not translated
    _currentBarStartToEnd = glm::vec2(m * glm::vec4(_originalBarStartToEnd, 0.0f, 0.0f));

    // ditto for emission direction because it is relative to the bar
    glm::vec2 newEmissionDir = glm::vec2(m * glm::vec4(_originalEmitDirection, 0.0f, 0.0f));
    _velocityCalculator.SetDir(newEmissionDir);
}
