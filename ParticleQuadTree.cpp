
#include "ParticleQuadTree.h"
#include "glload/include/glload/gl_4_4.h"   // for GL draw style in GenerateGeometry(...)
#include "glm/detail/func_geometric.hpp" // for normalizing glm vectors




// TODO: check performance
// TODO: after performance check
//  - remove all "neighbor" indices
//  - during particle-particle collision checking, check if a particle's region of influence extends beyond its current node, and if so:
//      - call "which node is this?"(int x, int y) to find the neighbor
//      - if no neighbor (off edge of quad tree), then do nothing and continue to the next particle check
//      - check if the neighbor has been checked before (possible for the top, top left, and top right neighbors to be same neighboring node), and if not, do the particle check on that region and record the neighbor as having been checked
// TODO: compare result with have explicit neighbor values
// TODO: all is done on the CPU side now, so write up a list of the things that the compute shader needs to do







/*-----------------------------------------------------------------------------------------------
Description:
    Gives members their default values.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
ParticleQuadTree::ParticleQuadTree() :
    _numNodesInUse(0),
    _particleRegionRadius(0.0f)
{
    // other structures already have initializers to 0
}

/*-----------------------------------------------------------------------------------------------
Description:
    Sets up the initial tree subdivision with _NUM_ROWS_IN_TREE_INITIAL by 
    _NUM_COLUMNS_IN_TREE_INITIAL nodes.  Boundaries are determined by the particle region center 
    and the particle region radius.  The ParticleUpdater should constrain particles to this 
    region, and the quad tree will subdivide within this region.

    Note: I considered and heavily entertained the idea of starting every frame with one node 
    and subdividing as necessary, but I abandoned that idea because there will be many particles 
    in all but the initial frames.  It took some extra calculations up front, but the initial 
    subdivision should cut down on the subdivisions that are needed on every frame.
Parameters:
    particleRegionCenter    In world space
    particleRegionRadius    In world space
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleQuadTree::InitializeTree(const glm::vec2 &particleRegionCenter, float particleRegionRadius)
{
    _particleRegionCenter = particleRegionCenter;
    _particleRegionRadius = particleRegionRadius;

    // starting in the upper left corner of the 2D particle region, where X is minimal and Y is 
    // maximal because OpenGL's rectangle origins are the bottom left corner, unlike most 
    // rectangles in graphical programming, where the top left is min x and min y
    float xBegin = particleRegionCenter.x - particleRegionRadius;
    float yBegin = particleRegionCenter.y + particleRegionRadius;

    float xIncrementPerNode = 2.0f * particleRegionRadius / _NUM_COLUMNS_IN_TREE_INITIAL;
    float yIncrementPerNode = 2.0f * particleRegionRadius / _NUM_ROWS_IN_TREE_INITIAL;

    float y = yBegin;
    for (int row = 0; row < _NUM_ROWS_IN_TREE_INITIAL; row++)
    {
        float x = xBegin;
        for (int column = 0; column < _NUM_COLUMNS_IN_TREE_INITIAL; column++)
        {
            int nodeIndex = (row * _NUM_COLUMNS_IN_TREE_INITIAL) + column;
            QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];
            node._inUse = true;

            // set the borders of the node
            node._leftEdge = x;
            node._rightEdge = x + xIncrementPerNode;
            node._topEdge = y;
            node._bottomEdge = y - yIncrementPerNode;

            // assign neighbors
            // Note: Nodes on the edge of the initial tree have three null neighbors.  There may 
            // be other ways to calculate these, but I chose the following because it is spelled 
            // out verbosely.
            // Ex: a top - row node has null top left, top, and top right neighbors.

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
        }

        // end of row, increment to the next one
        y -= yIncrementPerNode;
    }

    _numNodesInUse = _NUM_STARTING_NODES;


}

/*-----------------------------------------------------------------------------------------------
Description:
    Resets the tree to only use _NUM_ROWS_IN_TREE_INITIAL by _NUM_COLUMNS_IN_TREE_INITIAL nodes.
    After this, any calls to SubdivideNode(...) will run over previously subdivided nodes.

    Ex: Start with 64 nodes, each with calculated bounds.  There are 256 nodes total.  Calling
    Reset() will set the number of nodes in use back to 64.  The next SubdivideNode(...) will 
    then modify nodes 65, 66, 67, and 68.  Their previous values will be run over.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleQuadTree::ResetTree()
{
    for (int nodeIndex = 0; nodeIndex < _MAX_NODES; nodeIndex++)
    {
        QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];
        node._numCurrentParticles = 0;
        
        // not subdivided
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

/*-----------------------------------------------------------------------------------------------
Description:
    Governs the addition of particles to the quad tree.  It calculates which of the default tree 
    nodes the particle is in, and then adds the particle to it.  AddParticleToNode(...) will 
    handle subdivision and addition of particles to child nodes.
Parameters: 
    particleCollection  A container for all particles in use by this program.
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleQuadTree::AddParticlestoTree(std::vector<Particle> &particleCollection)
{
    float xIncrementPerColumn = 2.0f * _particleRegionRadius / _NUM_COLUMNS_IN_TREE_INITIAL;
    float yIncrementPerRow = 2.0f * _particleRegionRadius / _NUM_ROWS_IN_TREE_INITIAL;

    // is inverted to save on division cost for every particle on every frame
    float inverseXIncrementPerColumn = 1.0f / xIncrementPerColumn;
    float inverseYIncrementPerRow = 1.0f / yIncrementPerRow;

    for (size_t particleIndex = 0; particleIndex < particleCollection.size(); particleIndex++)
    {
        Particle &p = particleCollection[particleIndex];
        if (p._isActive == 0)
        {
            // only add active particles
            continue;
        }

        // I can't think of an intuitive explanation for why the following math works, but it 
        // does (I worked it out by hand, got it wrong, experimented, and got it right)
        // Note: Row bounds are on the Y axis, column bounds are on the X axis.  I always get 
        // them mixed up because a row is horizontal and a column is vertical.
        // Also Note:
        //  col index   = (int)((p.pos.x - quadTreeLeftEdge) / xIncrementPerColumn)
        //  row index   = (int)((quadTreeTopEdge - p.pos.y) / yIncrementPerRow);
        //
        //  Let c = particle region center
        //  Let r = particle region radius
        //  Let p = particle
        //  Then:
        //  col index = (int)((p.pos.x - (c.x - r)) / xIncrementPerColumn);
        //  row index = (int)(((c.y + r) - p.pos.y) / yIncrementPerRow);
        //
        // Also Also Note: The integer rounding should NOT be to the nearest integer.  Array 
        // indices start at 0, so any value between 0 and 1 is considered to be in the 0th index.

        // column 
        float leftEdge = _particleRegionCenter.x - _particleRegionRadius;
        float xDiff = p._position.x - leftEdge;
        float colFloat = xDiff * inverseXIncrementPerColumn;
        int colInt = int(colFloat);

        float topEdge = _particleRegionCenter.y + _particleRegionRadius;
        float yDiff = topEdge - p._position.y;
        float rowFloat = yDiff * inverseYIncrementPerRow;
        int rowInt = int(rowFloat);

        // same index calulation as in InitializeTree(...)
        int nodeIndex = (rowInt * _NUM_COLUMNS_IN_TREE_INITIAL) + colInt;
        AddParticleToNode(particleIndex, nodeIndex, particleCollection);
    }
}

// TODO: header
void ParticleQuadTree::DoTheParticleParticleCollisions(std::vector<Particle> &particleCollection, float deltaTimeSec) const
{
    for (int nodeIndex = 0; nodeIndex < _numNodesInUse; nodeIndex++)
    {
        if (nodeIndex == 19)
        {
            printf("");
        }
        const QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];

        if (!node._inUse)
        {
            continue;
        }
        else if (node._isSubdivided)
        {
            // check the children
            ParticleCollisionsWithinNode(node._childNodeIndexTopLeft, deltaTimeSec, particleCollection);
            ParticleCollisionsWithinNode(node._childNodeIndexTopRight, deltaTimeSec, particleCollection);
            ParticleCollisionsWithinNode(node._childNodeIndexBottomRight, deltaTimeSec, particleCollection);
            ParticleCollisionsWithinNode(node._childNodeIndexBottomLeft, deltaTimeSec, particleCollection);
        }
        else if (!node._numCurrentParticles == 0)
        {
            // check for collisions within this node only
            ParticleCollisionsWithinNode(nodeIndex, deltaTimeSec, particleCollection);
        }
    }
}

/*-----------------------------------------------------------------------------------------------
Description:
    Generates lines for the bounds of all nodes in use.  Used to draw a visualization of the 
    quad tree.

    Note: There are many duplicate nodes and lines, but this is just a demo.  I won't concern 
    myself with trying to optimize this aspect of the program.
Parameters: 
    putDataHere     Self-explanatory
    firstTime       If true, initializes the geometry data.  Dummy data is given.
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleQuadTree::GenerateGeometry(GeometryData *putDataHere, bool firstTime)
{
    // the data changes potentially every frame, so have to clear out the existing data
    putDataHere->_verts.clear();
    putDataHere->_indices.clear();

    // give it a load of dummy data the first time that this is called
    // Note: The first time that this is called is during setup so that the GeometryData object 
    // can allocate a correctly sized vertex buffer on the GPU.  The vertex buffer needs the 
    // buffer to be as large as it could possibly be at this time.  I don't want to have to deal 
    // with resizing the buffer at runtime when I could easily give it a maximum size at the 
    // start.
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
        for (int nodeCounter = 0; nodeCounter < _numNodesInUse; nodeCounter++)
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

    // used for glBufferData(...) and glBufferSubData(...)
    putDataHere->_vertexBufferSizeBytes = putDataHere->_verts.size() * sizeof(putDataHere->_verts[0]);
    putDataHere->_elementBufferSizeBytes = putDataHere->_indices.size() * sizeof(putDataHere->_indices[0]);
}

/*-----------------------------------------------------------------------------------------------
Description:
    A simple getter.  Used when printing the number of current quad tree nodes to the screen.

    Note: _numNodesInUse is modified in InitializeTree(...), ResetTree(), and SubdivideNode(...).
Parameters: Node
Returns:    
    The number of nodes that are currently in use by the tree (usually less than _MAX_NODES).
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
int ParticleQuadTree::NumNodesInUse() const
{
    return _numNodesInUse;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Adds the specified particle to the specified node.  If the new particle will push the node 
    over its particle containment limit, then it subdivides and populates its new children with 
    its own particles.

    Note: I could have passed the arguments by reference, but I'm keeping them as indices as a 
    reminder for myself of how to pass values in the GPU version.  Compute shaders don't have 
    pointers or references, so their must used indices.

    Also Note: This function does not sanitize the input.  The purpose of this function is to 
    create a connection between a particle and a node.  Be nice to it and check particle bounds 
    first.
    TODO: particles should not contain a link to its node; particle collisions will be calculated for each particle within a node, not for each particle overall

Parameters: 
    particleIndex   The particle that needs to be added.
    nodeIndex       The quad tree node to add the particle to.
    particleCollection  Self-explanatory
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
bool ParticleQuadTree::AddParticleToNode(int particleIndex, int nodeIndex, std::vector<Particle> &particleCollection)
{
    Particle &p = particleCollection[particleIndex];
    QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];
    int destinationNodeIndex = -1;

    if (node._isSubdivided == 0)
    {
        // not subdivided (yet), so add the particle to the provided node
        int numParticlesThisNode = node._numCurrentParticles;
        if (numParticlesThisNode == MAX_PARTICLES_PER_QUAD_TREE_NODE)
        {
            // ran out of space, so split the node and add the particles to its children
            if (!SubdivideNode(nodeIndex, particleCollection))
            {
                // no space left
                return false;
            }

            //node._numCurrentParticles = 0;
            // add the particle to this same node again, which will use the "is subdivided" logic
            destinationNodeIndex = nodeIndex;
        }
        else
        {
            // END RECURSION
            // Note: It's ok to use this as an index because the "== MAX" check was already done.
            node._indicesForContainedParticles[numParticlesThisNode] = particleIndex;
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
        }
    }

    return AddParticleToNode(particleIndex, destinationNodeIndex, particleCollection);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Grabs four unused nodes from the "all nodes" array, sets their bounds as four quadrants of 
    the parent node that needs to be subdivided, populates them with that node's particles, and 
    empties the parent node.
    // TODO: get rid of "neighbors" 
Parameters: 
    nodeIndex       The quad tree node to split
    particleCollection  Self-explanatory.
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
-----------------------------------------------------------------------------------------------*/
bool ParticleQuadTree::SubdivideNode(int nodeIndex, std::vector<Particle> &particleCollection)
{
    QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];

    if (_numNodesInUse > (_MAX_NODES - 4))
    {
        // not enough to nodes to subdivide again
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
    for (int particleCount = 0; particleCount < node._numCurrentParticles; particleCount++)
    {
        int particleIndex = node._indicesForContainedParticles[particleCount];
        Particle &p = particleCollection[particleIndex];

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

        AddParticleToNode(particleIndex, childNodeIndex, particleCollection);

        // not actually necessary because the array will be run over on the next update, but I 
        // still like to clean up after myself in case of debugging
        node._indicesForContainedParticles[particleCount] = -1;
    }

    node._numCurrentParticles = 0;

    // all went well
    return true;
}


// TODO: header
void ParticleQuadTree::ParticleCollisionsWithinNode(int nodeIndex, float deltaTimeSec, std::vector<Particle> &particleCollection) const
{
    const QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];

    if (node._isSubdivided)
    {
        // only check the children
        ParticleCollisionsWithinNode(node._childNodeIndexTopLeft, deltaTimeSec, particleCollection);
        ParticleCollisionsWithinNode(node._childNodeIndexTopRight, deltaTimeSec, particleCollection);
        ParticleCollisionsWithinNode(node._childNodeIndexBottomRight, deltaTimeSec, particleCollection);
        ParticleCollisionsWithinNode(node._childNodeIndexBottomLeft, deltaTimeSec, particleCollection);

        return;
    }

    // check all particles in the node for collisions against all other particles in the node
    // Note: The -1 prevents array overrun with the inner loop that starts at 
    // particleCount + 1.
    for (int particleCount = 0;
        particleCount < (node._numCurrentParticles - 1);
        particleCount++)
    {
        int particle1Index = node._indicesForContainedParticles[particleCount];

        // do not do an N^2 solution or else there will be duplicate particle-particle 
        // calculations
        // Note: The particle-particle collisions calulate the force applied by p1 on p2 and 
        // by p2 on p1.  To prevent duplicate calculations, start the following loop at the 
        // next particle in the node.  This approach makes sure that any two particles are 
        // only compared once.  
        for (int particleCompareCount = particleCount + 1;
            particleCompareCount < node._numCurrentParticles;
            particleCompareCount++)
        {
            int particle2Index = node._indicesForContainedParticles[particleCompareCount];

            ParticleCollisionP1WithP2(particle1Index, particle2Index, deltaTimeSec, particleCollection);
        }

        // check against all 8 neighbors
        // Note: If a particle is in a corner of a small particle region, it is possible for its 
        // region of influence to extend into multiple neighbors, so just use if(...) and not 
        // else if(...).
        Particle &p1 = particleCollection[particle1Index];

        float x = p1._position.x;
        float y = p1._position.y;
        float r = p1._radiusOfInfluence;

        // sin(45) == cos(45) == sqrt(2) / 2
        // Note: The particle's region of influence is circular.  Use a radius value modified by 
        // sin(45) to check whether the diagonal radius extends into a neighbor.
        float sqrt2Over2 = 0.70710678118f;
        float diagonalR = p1._radiusOfInfluence * sqrt2Over2;

        // remember that y increases from top to bottom (y = 0 at top, y = 1 at bottom)
        bool xWithinThisNode = (x > node._leftEdge) && (x < node._rightEdge);
        bool xLeft = x - r < node._leftEdge;
        bool xRight = x + r > node._rightEdge;
        bool xDiagonalLeft = x - diagonalR < node._leftEdge;
        bool xDiagonalRight = x + diagonalR > node._leftEdge;

        bool yWithinThisNode = (y > node._topEdge) && (y < node._bottomEdge);
        bool yTop = y - r < node._topEdge;
        bool yBottom = y + r > node._bottomEdge;
        bool yDiagonalTop = y - diagonalR < node._topEdge;
        bool yDiagonalBottom = y + diagonalR > node._bottomEdge;

        bool topLeft = xDiagonalLeft && yDiagonalTop;
        bool top = xWithinThisNode && yTop;
        bool topRight = xDiagonalRight && yDiagonalTop;
        bool right = xRight && yWithinThisNode;
        bool bottomRight = xDiagonalRight && yDiagonalBottom;
        bool bottom = xWithinThisNode && yBottom;
        bool bottomLeft = xDiagonalLeft && yDiagonalBottom;
        bool left = xLeft && yWithinThisNode;

        if (topLeft)
        {
            ParticleCollisionsWithNeighboringNode(particleCount, node._neighborIndexTopLeft, deltaTimeSec, particleCollection);
        }

        if (top)
        {
            ParticleCollisionsWithNeighboringNode(particleCount, node._neighborIndexTop, deltaTimeSec, particleCollection);
        }

        if (topRight)
        {
            ParticleCollisionsWithNeighboringNode(particleCount, node._neighborIndexTopRight, deltaTimeSec, particleCollection);
        }

        if (right)
        {
            ParticleCollisionsWithNeighboringNode(particleCount, node._neighborIndexRight, deltaTimeSec, particleCollection);
        }

        if (bottomRight)
        {
            ParticleCollisionsWithNeighboringNode(particleCount, node._neighborIndexBottomRight, deltaTimeSec, particleCollection);
        }

        if (bottom)
        {
            ParticleCollisionsWithNeighboringNode(particleCount, node._neighborIndexBottom, deltaTimeSec, particleCollection);
        }

        if (bottomLeft)
        {
            ParticleCollisionsWithNeighboringNode(particleCount, node._neighborIndexBottomLeft, deltaTimeSec, particleCollection);
        }

        if (left)
        {
            ParticleCollisionsWithNeighboringNode(particleCount, node._neighborIndexLeft, deltaTimeSec, particleCollection);
        }
    }
}

// TODO: header
void ParticleQuadTree::ParticleCollisionsWithNeighboringNode(int particleIndex, int nodeIndex, float deltaTimeSec, std::vector<Particle> &particleCollection) const
{
    const QuadTreeNode &node = _allQuadTreeNodes[nodeIndex];

    for (int particleCompareCount = 0;
        particleCompareCount < node._numCurrentParticles;
        particleCompareCount++)
    {
        int particle2Index = node._indicesForContainedParticles[particleCompareCount];

        ParticleCollisionP1WithP2(particleIndex, particle2Index, deltaTimeSec, particleCollection);
    }

}

float inline FastInverseSquareRoot(float x)
{
    float xHalf = 0.5f * x;
    int i = *(int *)&x;
    i = 0x5f3759df - (i >> 1);
    x = *(float *)&i;
    x = x *(1.5f - (xHalf * x * x));

    return x;
}

// TODO: header
// Calculates and adds force for P1 on P2 and P2 on P1.  An N^2 particle collision approach will result in duplicate force calculations.  
void ParticleQuadTree::ParticleCollisionP1WithP2(int p1Index, int p2Index, float deltaTimeSec, std::vector<Particle> &particleCollection) const
{
    Particle &p1 = particleCollection[p1Index];
    Particle &p2 = particleCollection[p2Index];

    glm::vec2 p1ToP2 = p2._position - p1._position;

    // partial pythagorean theorem
    float distanceBetweenSqr = (p1ToP2.x * p1ToP2.x) + (p1ToP2.y * p1ToP2.y);

    float minDistanceForCollisionSqr = (p1._radiusOfInfluence + p2._radiusOfInfluence);
    minDistanceForCollisionSqr = minDistanceForCollisionSqr * minDistanceForCollisionSqr;

    if (distanceBetweenSqr < minDistanceForCollisionSqr)
    {
        // elastic collision with conservation of momentum
        // Note: For an elastic collision between two particles of equal mass, the 
        // velocities of the two will be exchanged.  I could use this simplified 
        // idea for this demo, but I want to eventually have the option of different 
        // masses of particles, so I will use the general case elastic collision 
        // calculations (bottom of page at link).
        // http://hyperphysics.phy-astr.gsu.edu/hbase/colsta.html

        // for elastic collisions between two masses (ignoring rotation because 
        // these particles are points), use the calculations from this article (I 
        // followed them on paper too and it seems legit)
        // http://www.gamasutra.com/view/feature/3015/pool_hall_lessons_fast_accurate_.php?page=3

        //glm::vec2 normalizedLineOfContact = glm::normalize(p1ToP2);
        float lineOfContactMagnitudeSqr = glm::dot(p1ToP2, p1ToP2);
        glm::vec2 normalizedLineOfContact = p1ToP2 * FastInverseSquareRoot(lineOfContactMagnitudeSqr);

        float a1 = glm::dot(p1._velocity, p1ToP2);
        float a2 = glm::dot(p2._velocity, p1ToP2);

        // ??what else do I call it??
        float fraction = (2.0f * (a1 - a2)) / (p1._mass + p2._mass);

        // keep the intermediate "prime" values around for debugging
        glm::vec2 v1Prime = p1._velocity - (fraction * p2._mass) * normalizedLineOfContact;
        glm::vec2 v2Prime = p2._velocity + (fraction * p1._mass) * normalizedLineOfContact;

        glm::vec2 p1InitialMomentum = p1._velocity * p1._mass;
        glm::vec2 p2InitialMomentum = p2._velocity * p2._mass;
        glm::vec2 p1FinalMomentum = v1Prime * p1._mass;
        glm::vec2 p2FinalMomentum = v2Prime * p2._mass;

        // delta momentum (impulse) = force * delta time
        // therefore force = delta momentum / delta time
        glm::vec2 p1Force = (p1FinalMomentum - p1InitialMomentum) / deltaTimeSec;
        glm::vec2 p2Force = (p2FinalMomentum - p2InitialMomentum) / deltaTimeSec;

        //p1._velocity = v1Prime;
        //p2._velocity = v2Prime;
        p1._netForce += p1Force;
        p2._netForce += p2Force;

        p1._collisionCountThisFrame += 1;
        p2._collisionCountThisFrame += 1;
    }
}
