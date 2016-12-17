
#include "ParticleQuadTree.h"
#include "glload/include/glload/gl_4_4.h"   // for GL draw style in GenerateGeometry(...)




// a temporary helper value
// Note: In the GPU version, the "all particles" array will be readily available as a global.  The GPU cannot pass items by reference or pointer, so to avoid having this CPU version of the algorithm drift too much from the GPU version, I will use this pointer as a temporary value that will mimic the compute shader's global "all particles".
static std::vector<Particle> *allParticles = 0;



// TODO: add public method: void "get tree lines"(vector<MyVertex> *putDataHere) that spits a collection of lines (2-vertex pairs) that describe the quad trees; this is for drawing, and also also gives the number of nodes in use)
// TODO: particles don't need to know which node they are a part of, so the global "all particles" pointer is unnecessary; 
//  - get rid of it
//  - have "add particles to tree" take a const vector reference, not a non-const pointer
// TODO: add public method: int "which node is this?"(int x, int y)
// TODO: add public method: get const quad tree reference (the particle updater will perform the collision detection)
// TODO: cleanup
// TODO: function header blocks
// TODO: remove particle regions, replace with a simple radius
// TODO: replace _childTopLeft, _childTopRight, etc. with an array of child pointers, then pick betweent hem with an enum
// TODO: check performance
// TODO: after performance check
//  - remove all "neighbor" indices
//  - during particle-particle collision checking, check if a particle's region of influence extends beyond its current node, and if so:
//      - call "which node is this?"(int x, int y) to find the neighbor
//      - if no neighbor (off edge of quad tree), then do nothing and continue to the next particle check
//      - check if the neighbor has been checked before (possible for the top, top left, and top right neighbors to be same neighboring node), and if not, do the particle check on that region and record the neighbor as having been checked
// TODO: compare result with have explicit neighbor values
// TODO: all is done on the CPU side now, so write up a list of the things that the compute shader needs to do







// TODO: header
ParticleQuadTree::ParticleQuadTree() :
    _numNodesInUse(0),
    _particleRegionRadius(0.0f)
{
    // other structures already have initializers to 0
}



// TODO: header
// Note: I considered and heavily entertained the idea of starting every frame with one node and subdividing as necessary, but I abandoned that idea because of the additional load necessary to determine neighboring nodes during the subdivision.  By starting with a pre-subdivided array, I have neighboring nodes already in place, so when subdividing, the child nodes don't need to calculate anything new, but rather simply pull from their parents' information.
void ParticleQuadTree::InitializeTree(const glm::vec2 &particleRegionCenter, float particleRegionRadius)
{
    _particleRegionCenter = particleRegionCenter;
    _particleRegionRadius = particleRegionRadius;

    // starting in the upper left corner of the 2D particle region, where X is minimal and Y is maximal.
    float x = particleRegionCenter.x - particleRegionRadius;
    float y = particleRegionCenter.y + particleRegionRadius;

    float xIncrementPerNode = 2.0f * particleRegionRadius / _NUM_COLUMNS_IN_TREE_INITIAL;
    float yIncrementPerNode = 2.0f * particleRegionRadius / _NUM_ROWS_IN_TREE_INITIAL;

    for (int row = 0; row < _NUM_ROWS_IN_TREE_INITIAL; row++)
    {
        for (int column = 0; column < _NUM_COLUMNS_IN_TREE_INITIAL; column++)
        {
            int nodeIndex = (row * _NUM_COLUMNS_IN_TREE_INITIAL) + column;
            QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];

            // set the borders of the node
            node._leftEdge = x;
            node._rightEdge = x + xIncrementPerNode;
            node._topEdge = y;
            node._bottomEdge = y - yIncrementPerNode;

            // assign neighbors
            // Note: Nodes on the edge of the initial tree have three null neighbors (ex: a top-row node has null top left, top, and top right neighbors).  There may be other ways to calculate these, but I chose the following because it is spelled out verbosely.

            // left edge does not have a "left" neighbor
            if (column > 0)
            {
                node._neighborIndexLeft = nodeIndex - 1;
            }

            // left and top edges do not have a "top left" neighbor
            if (row > 0 && column > 0)
            {
                // "top left" neighbor = current node - 1 row - 1 column
                node._neighborIndexTopLeft = nodeIndex - _NUM_COLUMNS_IN_TREE_INITIAL - 1;
            }

            // top row does not have a "top" neighbor
            if (row > 0)
            {
                node._neighborIndexTop = nodeIndex - _NUM_COLUMNS_IN_TREE_INITIAL;
            }

            // right and top edges do not have a "top right" neighbor
            if (row > 0 && column < (_NUM_COLUMNS_IN_TREE_INITIAL - 1))
            {
                // "top right" neighbor = current node - 1 row + 1 column
                node._neighborIndexTopRight = nodeIndex - _NUM_COLUMNS_IN_TREE_INITIAL + 1;
            }

            // right edge does not have a "right" neighbor
            if (column < (_NUM_COLUMNS_IN_TREE_INITIAL - 1))
            {
                node._neighborIndexRight = nodeIndex + 1;
            }

            // right and bottom edges do not have a "bottom right" neighbor
            if (row < (_NUM_ROWS_IN_TREE_INITIAL - 1) &&
                column < (_NUM_COLUMNS_IN_TREE_INITIAL - 1))
            {
                // "bottom right" neighbor = current node + 1 row + 1 column
                node._neighborIndexBottomRight = nodeIndex + _NUM_COLUMNS_IN_TREE_INITIAL + 1;
            }

            // bottom edge does not have a "bottom" neighbor
            if (row < (_NUM_ROWS_IN_TREE_INITIAL - 1))
            {
                node._neighborIndexBottom = nodeIndex + _NUM_COLUMNS_IN_TREE_INITIAL;
            }

            // left and bottom edges do not have a "bottom left" neighbor
            if (row < (_NUM_ROWS_IN_TREE_INITIAL - 1) && column > 0)
            {
                // bottom left neighbor = current node + 1 row - 1 column
                node._neighborIndexBottomLeft = nodeIndex + _NUM_COLUMNS_IN_TREE_INITIAL - 1;
            }


            // setup for next node
            x += xIncrementPerNode;
            y -= yIncrementPerNode;
        }
    }

    _numNodesInUse = _NUM_STARTING_NODES;


}

// TODO: header
void ParticleQuadTree::ResetTree()
{
    for (int nodeIndex = 0; nodeIndex < _MAX_NODES; nodeIndex++)
    {
        QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];
        node._numCurrentParticles = 0;
        //node._startingParticleIndex = 0;
        node._isSubdivided = 0;
        node._childNodeIndexTopLeft = -1;
        node._childNodeIndexTopRight = -1;
        node._childNodeIndexBottomRight = -1;
        node._childNodeIndexBottomLeft = -1;

        // all excess nodes are turned off
        if (nodeIndex > _NUM_STARTING_NODES)
        {
            node._inUse = false;
        }
    }

    _numNodesInUse = _NUM_STARTING_NODES;
}

// TODO: header
// Note: This function does not sanitize the input.  The purpose of this function is to create a connection between a particle and a node.  Be nice to it and check particle bounds first.
bool ParticleQuadTree::AddParticleToNode(int particleIndex, int nodeIndex)
{
    Particle &p = (*allParticles)[particleIndex];
    QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];
    int destinationNodeIndex = -1;

    if (nodeIndex == -1)
    {
        printf("");
    }
    if (nodeIndex == 252)
    {
        printf("");
    }

    if (node._isSubdivided == 0)
    {
        // not subdivided (yet), so add the particle to the provided node
        int nodeParticleIndex = node._numCurrentParticles;
        if (nodeParticleIndex == MAX_PARTICLES_PER_QUAD_TREE_NODE)
        {
            // ran out of space, so split the node and add the particles to its children
            if (!SubdivideNode(nodeIndex))
            {
                // no space left
                return false;
            }

            //node._numCurrentParticles = 0;
            destinationNodeIndex = nodeIndex;

            if (destinationNodeIndex == -1)
            {
                printf("");
            }

        }
        else
        {
            // END RECURSION
            node._indicesForContainedParticles[nodeParticleIndex] = particleIndex;
            node._numCurrentParticles++;
            p._currentQuadTreeIndex = nodeIndex;
            return true;
        }
    }
    else
    {
        // the node is subdivided, so add the particle to the child nodes
        float nodeXCenter = (node._leftEdge + node._rightEdge) * 0.5f;
        float nodeYCenter = (node._bottomEdge + node._topEdge) * 0.5f;

        if (p._position.x < nodeXCenter)
        {
            // left half
            if (p._position.y > nodeYCenter)
            {
                // top half
                destinationNodeIndex = node._childNodeIndexTopLeft;
            }
            else
            {
                // bottom half 
                destinationNodeIndex = node._childNodeIndexBottomLeft;
            }

            if (destinationNodeIndex == -1)
            {
                printf("");
            }

        }
        else
        {
            // right half 
            if (p._position.y > nodeYCenter)
            {
                // top half 
                destinationNodeIndex = node._childNodeIndexTopRight;
            }
            else
            {
                // bottom half 
                destinationNodeIndex = node._childNodeIndexBottomRight;
            }

            if (destinationNodeIndex == -1)
            {
                printf("");
            }

        }
    }

    if (destinationNodeIndex == -1)
    {
        printf("");
    }
    return AddParticleToNode(particleIndex, destinationNodeIndex);
}


// TODO: header
bool ParticleQuadTree::SubdivideNode(int nodeIndex)
{
    QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];

    if (nodeIndex == 252)
    {
        printf("");
    }
    if (_numNodesInUse >= _MAX_NODES)
    {
        //fprintf(stderr, "number nodes in use '%d' maxed out (max = '%d')\n", _numNodesInUse, _MAX_NODES);
        return false;
    }

    int childNodeIndexTopLeft = _numNodesInUse++;
    int childNodeIndexTopRight = _numNodesInUse++;
    int childNodeIndexBottomRight = _numNodesInUse++;
    int childNodeIndexBottomLeft = _numNodesInUse++;
    node._isSubdivided = 1;
    node._childNodeIndexTopLeft = childNodeIndexTopLeft;
    node._childNodeIndexTopRight = childNodeIndexTopRight;
    node._childNodeIndexBottomRight = childNodeIndexBottomRight;
    node._childNodeIndexBottomLeft = childNodeIndexBottomLeft;

    float nodeXCenter = (node._leftEdge + node._rightEdge) * 0.5f;
    float nodeYCenter = (node._bottomEdge + node._topEdge) * 0.5f;

    // assign neighbors
    // Note: This is going to get really messy when I move to 3D and have to use octrees, each 
    // with 26 neighbors ((3 * 3 * 3) - 1 center node).

    QuadTreeNode &childTopLeft = _allQuadTreeNodes[childNodeIndexTopLeft];
    childTopLeft._neighborIndexLeft = node._neighborIndexLeft;
    childTopLeft._neighborIndexTopLeft = node._neighborIndexTopLeft;
    childTopLeft._neighborIndexTop = node._neighborIndexTop;
    childTopLeft._neighborIndexTopRight = node._neighborIndexTop;
    childTopLeft._neighborIndexRight = childNodeIndexTopRight;
    childTopLeft._neighborIndexBottomRight = childNodeIndexBottomRight;
    childTopLeft._neighborIndexBottom = childNodeIndexBottomLeft;
    childTopLeft._neighborIndexBottomLeft = node._neighborIndexLeft;
    childTopLeft._leftEdge = node._leftEdge;
    childTopLeft._topEdge = node._topEdge;
    childTopLeft._rightEdge = nodeXCenter;
    childTopLeft._bottomEdge = nodeYCenter;

    QuadTreeNode &childTopRight = _allQuadTreeNodes[childNodeIndexTopRight];
    childTopRight._neighborIndexLeft = childNodeIndexTopLeft;
    childTopRight._neighborIndexTopLeft = node._neighborIndexTop;
    childTopRight._neighborIndexTop = node._neighborIndexTop;
    childTopRight._neighborIndexTopRight = node._neighborIndexTopRight;
    childTopRight._neighborIndexRight = node._neighborIndexRight;
    childTopRight._neighborIndexBottomRight = node._neighborIndexRight;
    childTopRight._neighborIndexBottom = childNodeIndexBottomRight;
    childTopRight._neighborIndexBottomLeft = childNodeIndexBottomLeft;
    childTopRight._leftEdge = nodeXCenter;
    childTopRight._topEdge = node._topEdge;
    childTopRight._rightEdge = node._rightEdge;
    childTopRight._bottomEdge = nodeYCenter;

    QuadTreeNode &childBottomRight = _allQuadTreeNodes[childNodeIndexBottomRight];
    childBottomRight._neighborIndexLeft = childNodeIndexBottomLeft;
    childBottomRight._neighborIndexTopLeft = childNodeIndexTopLeft;
    childBottomRight._neighborIndexTop = childNodeIndexTopRight;
    childBottomRight._neighborIndexTopRight = node._neighborIndexRight;
    childBottomRight._neighborIndexRight = node._neighborIndexRight;
    childBottomRight._neighborIndexBottomRight = node._neighborIndexBottomRight;
    childBottomRight._neighborIndexBottom = node._neighborIndexBottom;
    childBottomRight._neighborIndexBottomLeft = node._neighborIndexBottom;
    childBottomRight._leftEdge = nodeXCenter;
    childBottomRight._topEdge = nodeYCenter;
    childBottomRight._rightEdge = node._rightEdge;
    childBottomRight._bottomEdge = node._bottomEdge;

    QuadTreeNode &childBottomLeft = _allQuadTreeNodes[childNodeIndexBottomLeft];
    childBottomLeft._neighborIndexLeft = node._neighborIndexLeft;
    childBottomLeft._neighborIndexTopLeft = node._neighborIndexLeft;
    childBottomLeft._neighborIndexTop = childNodeIndexTopLeft;
    childBottomLeft._neighborIndexTopRight = childNodeIndexTopRight;
    childBottomLeft._neighborIndexRight = childNodeIndexBottomRight;
    childBottomLeft._neighborIndexBottomRight = node._neighborIndexBottom;
    childBottomLeft._neighborIndexBottom = node._neighborIndexBottom;
    childBottomLeft._neighborIndexBottomLeft = node._neighborIndexBottomLeft;
    childBottomLeft._leftEdge = node._leftEdge;
    childBottomLeft._topEdge = nodeYCenter;
    childBottomLeft._rightEdge = nodeXCenter;
    childBottomLeft._bottomEdge = node._bottomEdge;

    // add all particles to children
    for (size_t particleCount = 0; particleCount < node._numCurrentParticles; particleCount++)
    {
        int particleIndex = node._indicesForContainedParticles[particleCount];
        Particle &p = (*allParticles)[particleIndex];

        // the node is subdivided, so add the particle to the child nodes
        int childNodeIndex = -1;
        if (p._position.x < nodeXCenter)
        {
            // left half of the node
            if (p._position.y > nodeYCenter)
            {
                // top half of the node
                childNodeIndex = node._childNodeIndexTopLeft;
            }
            else
            {
                // bottom half of the node
                childNodeIndex = node._childNodeIndexBottomLeft;
            }
        }
        else
        {
            // right half of the node
            if (p._position.y > nodeYCenter)
            {
                // top half of the node
                childNodeIndex = node._childNodeIndexTopRight;
            }
            else
            {
                // bottom half of the node
                childNodeIndex = node._childNodeIndexBottomRight;
            }
        }

        AddParticleToNode(particleIndex, childNodeIndex);

        // not actually necessary because the array will be run over on the next update, but I 
        // still like to clean up after myself in case of debugging
        node._indicesForContainedParticles[particleCount] = 0;
    }

    if (node._childNodeIndexTopLeft == -1)
    {
        printf("");
    }



    // all went well
    return true;
}


// TODO: header
void ParticleQuadTree::AddParticlestoTree(std::vector<Particle> *particleCollection)
{
    // TODO: get rid of this
    allParticles = particleCollection;

    float xIncrementPerNode = 2.0f * _particleRegionRadius / _NUM_COLUMNS_IN_TREE_INITIAL;
    float yIncrementPerNode = 2.0f * _particleRegionRadius / _NUM_ROWS_IN_TREE_INITIAL;

    // is inverted to save on division cost for every particle on every frame
    float inverseXIncrementPerNode = 1.0f / xIncrementPerNode;
    float inverseYIncrementPerNode = 1.0f / yIncrementPerNode;

    for (size_t particleIndex = 0; particleIndex < particleCollection->size(); particleIndex++)
    {
        if (particleIndex == 61)
        {
            printf("");
        }
        if (particleCollection->size() != 15000)
        {
            printf("");
        }
        Particle &p = (*particleCollection)[particleIndex];
        if (p._isActive == 0)
        {
            // only add active particles
            continue;
        }

        // I can't think of an intuitive explanation for why the following math works, but it 
        // does (I worked it out by hand)
        // Note: If the particle region was centered on 0, then the calculations are as follows:
        // Let r = particle region radius
        // Let p = particle
        // column index = (int)((p.pos.x + r) / X increment per node)
        // row index = (int)((p.pos.y + r) / Y increment per node)
        //
        // But if the particle region is not centered at (0,0), the calculation gets more complicated:
        // column index = (int)((p.pos.x - (regionCenter.X - r)) / X increment per node)
        // row index = (int)(((regionCenter.y + r) - p.pos.y) / Y increment per node)
        int row = (int)((p._position.x - (_particleRegionCenter.x - _particleRegionRadius)) * inverseXIncrementPerNode);
        int column = (int)(((_particleRegionCenter.y + _particleRegionRadius) - p._position.y) * inverseYIncrementPerNode);

        if (row < 0 || row > _NUM_ROWS_IN_TREE_INITIAL)
        {
            fprintf(stderr, "Error: calculated particle row '%d' out of range of initial number of rows [0 - %d]\n", row, _NUM_ROWS_IN_TREE_INITIAL);
        }
        if (column < 0 || column > _NUM_COLUMNS_IN_TREE_INITIAL)
        {
            fprintf(stderr, "Error: calculated particle column '%d' out of range of initial number of columns [0 - %d]\n", column, _NUM_COLUMNS_IN_TREE_INITIAL);
        }

        // follow the same index calulation as in InitializeTree(...)
        int nodeIndex = (row * _NUM_COLUMNS_IN_TREE_INITIAL) + column;

        if (particleIndex == 53)
        {
            printf("");
        }
        AddParticleToNode(particleIndex, nodeIndex);
    }

    printf("");
}

// TODO: header
// Note: There are many duplicate nodes and lines, but this is just a demo.  I won't concern myself with trying to optimize this rendering aspect of the program.
void ParticleQuadTree::GenerateGeometry(GeometryData *putDataHere, bool firstTime)
{
    // the data changes potentially every frame, so have to clear out the existing data
    putDataHere->_verts.clear();
    putDataHere->_indices.clear();

    // give it a load of dummy data the first time that this is called
    // Note: The first time that this is called is during setup so that the GeometryData object can allocate a correctly sized vertex buffer on the GPU.  The vertex buffer needs the buffer to be as large as it could possibly be at this time.  I don't want to have to deal with resizing the buffer at runtime when I could easily give it a maximum size at the start.
    if (firstTime)
    {
        putDataHere->_drawStyle = GL_LINES;

        // 4 corners per box
        putDataHere->_verts.resize(_MAX_NODES * 4);

        // 4 lines per box, 2 vertices per line
        putDataHere->_indices.resize(_MAX_NODES * 4 * 2);
    }
    else
    {
        unsigned short vertexIndex = 0;
        for (size_t nodeCounter = 0; nodeCounter < _numNodesInUse; nodeCounter++)
        {
            QuadTreeNode &node = _allQuadTreeNodes[nodeCounter];

            // 4 corners per box
            MyVertex topLeft;
            unsigned short topLeftIndex = vertexIndex++;
            topLeft._position = glm::vec2(node._leftEdge, node._topEdge);
            putDataHere->_verts.push_back(topLeft);

            MyVertex topRight;
            unsigned short topRightIndex = vertexIndex++;
            topRight._position = glm::vec2(node._rightEdge, node._topEdge);
            putDataHere->_verts.push_back(topRight);

            MyVertex bottomRight;
            unsigned short bottomRightIndex = vertexIndex++;
            bottomRight._position = glm::vec2(node._rightEdge, node._bottomEdge);
            putDataHere->_verts.push_back(bottomRight);

            MyVertex bottomLeft;
            unsigned short bottomeLeftIndex = vertexIndex++;
            bottomLeft._position = glm::vec2(node._leftEdge, node._bottomEdge);
            putDataHere->_verts.push_back(bottomLeft);

            // 4 lines per box, 2 vertices per line
            // Note: These are lines, so there is no concern about clockwise or counterclockwise
            putDataHere->_indices.push_back(topLeftIndex);
            putDataHere->_indices.push_back(topRightIndex);
            putDataHere->_indices.push_back(topRightIndex);
            putDataHere->_indices.push_back(bottomRightIndex);
            putDataHere->_indices.push_back(bottomRightIndex);
            putDataHere->_indices.push_back(bottomeLeftIndex);
            putDataHere->_indices.push_back(bottomeLeftIndex);
            putDataHere->_indices.push_back(topLeftIndex);
        }

    }

}