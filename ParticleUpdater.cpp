#include "ParticleUpdater.h"

const unsigned int MAX_PARTICLES_PER_QUAD_TREE_NODE = 50;

struct QuadTreeNode
{
    QuadTreeNode() :
        _numCurrentParticles(0),
        _startingParticleIndex(0),
        _inUse(0),
        _leftEdge(0.0f),
        _topEdge(0.0f),
        _rightEdge(0.0f),
        _bottomEdge(0.0f),
        _neighborIndexLeft(-1),
        _neighborIndexUpLeft(-1),
        _neighborIndexUp(-1),
        _neighborIndexUpright(-1),
        _neighborIndexRight(-1),
        _neighborIndexDownRight(-1),
        _neighborIndexDown(-1),
        _neighborIndexDownLeft(-1)
    {
        memset(_indicesForContainedParticles, 0, sizeof(int) * MAX_PARTICLES_PER_QUAD_TREE_NODE);
    }

    int _indicesForContainedParticles[MAX_PARTICLES_PER_QUAD_TREE_NODE];
    int _numCurrentParticles;

    // for use in GPU version
    int _startingParticleIndex;
    int _inUse;

    // left and right edges implicitly X, top and bottom implicitly Y
    float _leftEdge;
    float _topEdge;
    float _rightEdge;
    float _bottomEdge;

    int _neighborIndexLeft;
    int _neighborIndexUpLeft;
    int _neighborIndexUp;
    int _neighborIndexUpright;
    int _neighborIndexRight;
    int _neighborIndexDownRight;
    int _neighborIndexDown;
    int _neighborIndexDownLeft;

};

// increase the number of additional nodes as necessary to handle more subdivision
const int NUM_ROWS_IN_TREE_INITIAL = 8;
const int NUM_COLUMNS_IN_TREE_INITIAL = 8;
const int NUM_STARTING_NODES = NUM_ROWS_IN_TREE_INITIAL * NUM_COLUMNS_IN_TREE_INITIAL;
const int MAX_NODES = NUM_STARTING_NODES * 4;
QuadTreeNode allQuadTreeNodes[MAX_NODES];

// TODO: On GPU version, start with an 8x8 array of nodes and have a work item operating on each possible node

// TODO: header
// Note: I considered and heavily entertained the idea of starting every frame with one node and subdividing as necessary, but I abandoned that idea because of the additional load necessary to determine neighboring nodes during the subdivision.  By starting with a pre-subdivided array
void InitializeTree(const glm::vec2 &particleRegionCenter, float particleRegionRadius)
{
    float xIncrement = 2 * particleRegionRadius / NUM_COLUMNS_IN_TREE_INITIAL;
    float yIncrement = 2 * particleRegionRadius / NUM_ROWS_IN_TREE_INITIAL;

    // starting in the upper left corner of the 2D particle region, where X is minimal and Y is maximal.
    float x = particleRegionCenter.x - particleRegionRadius;
    float y = particleRegionCenter.y + particleRegionRadius;

    for (int nodeIndex = 0; nodeIndex < NUM_STARTING_NODES; nodeIndex++)
    {
        QuadTreeNode &nodeRef = allQuadTreeNodes[nodeIndex];

    }
}

// TODO: header
void AddParticlestoTree(std::vector<Particle> &particleCollection)
{

}





/*-----------------------------------------------------------------------------------------------
Description:
    Ensures that the object starts object with initialized values.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
ParticleUpdater::ParticleUpdater() :
    _pRegion(0)
{
    for (size_t emitterIndex = 0; emitterIndex < MAX_EMITTERS; emitterIndex++)
    {
        _pEmitters[emitterIndex] = 0;
        _maxParticlesEmittedPerFrame[emitterIndex] = 0;
    }
    _emitterCount = 0;
}

/*-----------------------------------------------------------------------------------------------
Description:
    A simple assignment. 
Parameters: 
    pRegion     A pointer to a "particle region" interface.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleUpdater::SetRegion(const IParticleRegion *pRegion)
{
    _pRegion = pRegion;
}

/*-----------------------------------------------------------------------------------------------
Description:
    A simple assignment.
Parameters: 
    pEmitter    A pointer to a "particle emitter" interface.
    maxParticlesEmittedPerFrame     A restriction on the provided emitter to prevent all 
        particles from being emitted at once.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleUpdater::AddEmitter(const IParticleEmitter *pEmitter, const int maxParticlesEmittedPerFrame)
{
    if (_emitterCount >= MAX_EMITTERS)
    {
        return;
    }

    _pEmitters[_emitterCount] = pEmitter;
    _maxParticlesEmittedPerFrame[_emitterCount] = maxParticlesEmittedPerFrame;
    _emitterCount++;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Checks if each particle is out of bounds, and if so, tells the emitter to reset it.  If the 
    emitter hasn't reached its quota for emitted particles, then the particle is sent back out 
    again.  Lastly, if the particle is active, then its position is updated with its velocity and
    the provided delta time.
Parameters:
    particleCollection  The particle collection that will be updated.
    startIndex          Used in case the user wanted to adapt the updater to use multiple 
                        emitters and then wanted to split the number of particles between these 
                        emitters.
    numToUpdate         Same idea as "start index".
    deltatimeSec        Self-explanatory
Returns:    
    The number of active particles.  Useful for performance comparison with GPU version.
Exception:  Safe
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
unsigned int ParticleUpdater::Update(std::vector<Particle> &particleCollection, 
    const unsigned int startIndex, const unsigned int numToUpdate, const float deltaTimeSec) const
{
    if (_emitterCount == 0 || _pRegion == 0)
    {
        return 0;
    }

    // for all particles:
    // - if it has gone out of bounds, reset it and deactivate it
    // - if it is inactive and the emitter hasn't used up its quota for emitted particles this frame, reactivate it
    // - if it is active, update its position with its velocity
    // Note: If if() statements are used for each situation, then a particle has a chance to go 
    // out of bounds and get reset, get reactivated, and emit again in the same frame.  If 
    // else-if() statements are used, then only one of those situations will be run per frame.  
    // I did the former, but it doesn't really matter which approach is chosen.

    // simply called "end" because I want to keep using the "< end" notation on the loop end 
    // condition
    unsigned int endIndex = startIndex + numToUpdate;
    if (endIndex > particleCollection.size())
    {
        // if "end" was already == particle collection size, then all is good
        endIndex = particleCollection.size();
    }

    // when using multiple emitters, it looks best to cycle between all emitters one by one, but that is also more difficult to deal with and requires a number of different checks and conditions, and provided the total number of particles to update exceeds the total number of max particles emitted per frame across all emitters, then it will look just as good to "fill up" each emitter one by one, and that is much easier to implement
    unsigned int particleEmitCounter = 0;
    int emitterIndex = 0;
    unsigned int numActiveParticles = 0;

    for (size_t particleIndex = startIndex; particleIndex < endIndex; particleIndex++)
    {
        Particle &pCopy = particleCollection[particleIndex];
        if (_pRegion->OutOfBounds(pCopy))
        {
            pCopy._isActive = false;
        }

        if (pCopy._isActive)
        {
            numActiveParticles++;
            pCopy._position = pCopy._position + (pCopy._velocity * deltaTimeSec);
        }
        else if (emitterIndex < MAX_EMITTERS)   // also implicitly, "is active" is false
        {
            // if all emitters have put out all they can this frame, then this condition will 
            // not be entered
            _pEmitters[emitterIndex]->ResetParticle(&pCopy);
            pCopy._isActive = true;
            particleCollection[particleIndex] = pCopy;

            particleEmitCounter++;
            if (particleEmitCounter >= _maxParticlesEmittedPerFrame[emitterIndex])
            {
                // get the next emitter
                // Note: This is in a while loop because the number of emitters in this particle updater are unknown and some of the pointers in the emitter array may still be 0, so this has to loop through (potentially) all of them.
                emitterIndex++;
                while (emitterIndex < MAX_EMITTERS)
                {
                    if (_pEmitters[emitterIndex] != 0)
                    {
                        // got another one, so start the particle emission counter over again
                        particleEmitCounter = 0;
                        break;
                    }
                    emitterIndex++;
                }
            }
        }
    }

    return numActiveParticles;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Used during initialization to give all particles initial values.  It would not do to have 
    everyone with random values composed of whatever bits were already in the memory where they 
    ended up.
Parameters:
    particleCollection  Self-explanatory
Returns:    None
Exception:  Safe
Creator:    John Cox (8-13-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleUpdater::ResetAllParticles(std::vector<Particle> &particleCollection)
{
    // reset all particles evenly 
    // Note: I could do a weighted fancy algorithm and account for the "particles emitted per frame" for each emitter, but this is just a demo program.
    // Also Note: This integer division could leave a few particles unaffected, but those will quickly be swept up into the flow of things when "update" runs.
    unsigned int particlesPerEmitter = particleCollection.size() / _emitterCount;
    for (size_t emitterIndex = 0; emitterIndex < _emitterCount; emitterIndex++)
    {
        for (size_t particleIndex = 0; particleIndex < particlesPerEmitter; particleIndex++)
        {
            _pEmitters[emitterIndex]->ResetParticle(&particleCollection[particleIndex]);
        }
    }
}

