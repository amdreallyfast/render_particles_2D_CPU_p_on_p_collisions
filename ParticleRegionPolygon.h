//#pragma once
//
//#include "IParticleRegion.h"
//#include "glm/vec2.hpp"
//#include "Particle.h"
//#include <vector>
//
//
//// num corners == num faces
//const unsigned int MAX_POLYGON_FACES = 10;
//
///*-----------------------------------------------------------------------------------------------
//Description:
//    This object defines a polygonal region within which particles are considered active and the
//    logic to determine when a particle goes outside of its boundaries.
//Creator:    John Cox (7-2-2016)
//-----------------------------------------------------------------------------------------------*/
//class ParticleRegionPolygon : public IParticleRegion
//{
//public:
//    ParticleRegionPolygon(const std::vector<glm::vec2> &corners);
//    virtual bool OutOfBounds(const Particle &p) const;
//    virtual void SetTransform(const glm::mat4 &m);
//
//private:
//    unsigned int _numFaces;
//
//    // do NOT use a std::vector or any other container that allocates memory on the heap
//    // Note: Tinkering last weekend discovered that using a std::vector<...> trashed the data 
//    // cache when running the "out of bounds" check.  The continual need to run out to the heap 
//    // just to find the corners' vectors created a major frame rate drop.
//    // Solution: Use arrays with a max size.  This keeps up the flexibility of variable number of
//    // sides while keeping all necessary info in the process' data memory.
//    glm::vec2 _originalFaceCenterPoints[MAX_POLYGON_FACES];
//    glm::vec2 _originalFaceNormals[MAX_POLYGON_FACES];
//    glm::vec2 _currentFaceCenterPoints[MAX_POLYGON_FACES];
//    glm::vec2 _currentFaceNormals[MAX_POLYGON_FACES];
//};