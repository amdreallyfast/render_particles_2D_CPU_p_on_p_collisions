//#include "ParticleRegionCircle.h"
//
//#include "glm/detail/func_geometric.hpp"    // glm::dot
//
///*-----------------------------------------------------------------------------------------------
//Description:
//    Ensures that the object starts object with initialized values.
//Parameters: 
//    center  The central point of the circle.  Self-explanatory.
//    radius  Specified (in this demo, at least) in window coordinates (X and Y on range [-1,+1]).
//Returns:    None
//Exception:  Safe
//Creator:    John Cox (7-2-2016)
//-----------------------------------------------------------------------------------------------*/
//ParticleRegionCircle::ParticleRegionCircle(const glm::vec2 &center, const float radius)
//{
//    // start these two equal, but if a transform is added, then they will diverge
//    _originalCenter = center;
//    _currentCenter = _originalCenter;
//
//    _radiusSqr = radius * radius;
//}
//
///*-----------------------------------------------------------------------------------------------
//Description:
//    Checks if the provided particle has gone outside the circle.
//Parameters:
//    p   A const reference to a Particle object.
//Returns:    
//    True if the particle's position is outside the circle's boundaries, otherwise false.
//Exception:  Safe
//Creator:    John Cox (7-2-2016)
//-----------------------------------------------------------------------------------------------*/
//bool ParticleRegionCircle::OutOfBounds(const Particle &p) const
//{
//    glm::vec2 centerToParticle = p._position - _currentCenter;
//    float distSqr = glm::dot(centerToParticle, centerToParticle);
//    if (distSqr > _radiusSqr)
//    {
//        return true;
//    }
//    else
//    {
//        return false;
//    }
//}
//
///*-----------------------------------------------------------------------------------------------
//Description:
//    Applies the transform to the circle's center point.
//Parameters:
//    m       A 4x4 transform matrix.  Because glm transform functions only spit out a 4x4.
//Returns:    None
//Exception:  Safe
//Creator:    John Cox (8-26-2016)
//-----------------------------------------------------------------------------------------------*/
//void ParticleRegionCircle::SetTransform(const glm::mat4 &m)
//{
//    _currentCenter = glm::vec2(m * glm::vec4(_originalCenter, 0.0f, 1.0f));
//}
