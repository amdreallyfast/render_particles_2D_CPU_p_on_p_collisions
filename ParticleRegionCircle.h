//#pragma once
//
//#include "IParticleRegion.h"
//#include "glm/vec2.hpp"
//#include "Particle.h"
//
///*-----------------------------------------------------------------------------------------------
//Description:
//    This object defines a circular region within which particles are considered active and the 
//    logic to determine when a particle goes outside of its boundaries.
//Creator:    John Cox (7-2-2016)
//-----------------------------------------------------------------------------------------------*/
//class ParticleRegionCircle : public IParticleRegion
//{
//public:
//    ParticleRegionCircle(const glm::vec2 &center, const float radius);
//    virtual bool OutOfBounds(const Particle &p) const;
//    virtual void SetTransform(const glm::mat4 &m);
//
//private:
//    glm::vec2 _originalCenter;
//    glm::vec2 _currentCenter;   // for speed during calls to OutOfBounds(...)
//    float _radiusSqr;   // because radius is never used
//};
