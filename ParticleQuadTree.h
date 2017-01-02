#pragma once

#include <vector>
#include "Particle.h"
#include "glm\vec2.hpp"
#include "ParticleQuadTreeNode.h"
#include "GeometryData.h"

/*-----------------------------------------------------------------------------------------------
Description:
    Responsible for generating a quad tree that can contain all the currently active particles.
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
class ParticleQuadTree
{
public:
    ParticleQuadTree();
    void InitializeTree(const glm::vec2 &particleRegionCenter, float particleRegionRadius);
    void ResetTree();
    void AddParticlestoTree(std::vector<Particle> *particleCollection);
    
    void GenerateGeometry(GeometryData *putDataHere, bool firstTime = false);
    int NumNodesInUse() const;

private:
    bool AddParticleToNode(int particleIndex, int nodeIndex);
    bool SubdivideNode(int nodeIndex);

    // increase the number of additional nodes as necessary to handle more subdivision
    // Note: This algorithm was built with the compute shader's implementation in mind.  These 
    // structures are meant to be used as if a compute shader was running it, hence all the 
    // arrays and a complete lack of runtime memory reallocation.
    static const int _NUM_ROWS_IN_TREE_INITIAL = 8;
    static const int _NUM_COLUMNS_IN_TREE_INITIAL = 8;
    static const int _NUM_STARTING_NODES = _NUM_ROWS_IN_TREE_INITIAL * _NUM_COLUMNS_IN_TREE_INITIAL;
    static const int _MAX_NODES = _NUM_STARTING_NODES * 8;
    
    QuadTreeNode _allQuadTreeNodes[_MAX_NODES];
    int _numNodesInUse = _NUM_STARTING_NODES;
    glm::vec2 _particleRegionCenter;
    float _particleRegionRadius;
};


