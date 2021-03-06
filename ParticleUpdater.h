#pragma once

#include "Particle.h"
#include "IParticleEmitter.h"
#include <vector>
#include "glm/vec2.hpp"

/*-----------------------------------------------------------------------------------------------
Description:
    Encapsulates particle updating with a given emitter and region.  The main function is the 
    "update" method.

    Note: When this class goes "poof", it won't delete the given pointers.  This is ensured by
    only using const pointers.
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
class ParticleUpdater
{
public:
    ParticleUpdater();
    
    void SetRegion(const glm::vec2 &particleRegionCenter, const float particleRegionRadius);
    void AddEmitter(const IParticleEmitter *pEmitter, const int maxParticlesEmittedPerFrame);
    // no "remove emitter" method because this is just a demo

    void Update(std::vector<Particle> &particleCollection, const unsigned int startIndex, 
        const unsigned int numToUpdate, const float deltaTimeSec);
    unsigned int NumActiveParticles() const;
    void ResetAllParticles(std::vector<Particle> &particleCollection) const;

private:
    bool ParticleOutOfBounds(const Particle &p) const;

    // for future demos, the only region that is needed is a circle/sphere
    // Note: Future particle containment will be handled by particle-polygon collisions.
    // Also Note: Storing the square of the radius because that is easier than calculating the 
    // square of the radius for every active particle on every frame.
    glm::vec2 _particleRegionCenter;
    float _particleRegionRadiusSqr;

    // use arrays instead of std::vector<...> for the sake of cache coherency
    unsigned int _numActiveParticles;
    unsigned int _emitterCount;
    static const int MAX_EMITTERS = 5;
    const IParticleEmitter *_pEmitters[MAX_EMITTERS];
    unsigned int _maxParticlesEmittedPerFrame[MAX_EMITTERS];
};
