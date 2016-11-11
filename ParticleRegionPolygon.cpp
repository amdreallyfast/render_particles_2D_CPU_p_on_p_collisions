#include "ParticleRegionPolygon.h"

#include "glm/detail/func_geometric.hpp"    // for dot and normalize

/*-----------------------------------------------------------------------------------------------
Description:
    Encapsulates the rotating of a 2D vector by -90 degrees (+90 degrees not used in this demo).
Parameters:
    v   A const 2D vector.
Returns:    
    A 2D vector rotated -90 degrees from the provided one.
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
static glm::vec2 RotateNeg90(const glm::vec2 &v)
{
    return glm::vec2(v.y, -(v.x));
}

/*-----------------------------------------------------------------------------------------------
Description:
    Ensures that the object starts object with initialized values.  Restricted to number of 
    corners equivalent to MAX_POLYGON_FACES.
Parameters:
    corners     A counterclockwise collection of 2D points in window space (XY on range[-1,+1]).
                If the number of corners exceeds MAX_POLYGON_FACES, extra corners are ignored.
                ??throw exception instead??
Returns:    None
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
ParticleRegionPolygon::ParticleRegionPolygon(const std::vector<glm::vec2> &corners)
{

    //_numCorners = corners.size();
    
    // 3 corners -> triangle -> 3 faces
    // 4 corners -> square -> 4 faces
    // ...
    // num corners == num faces
    _numFaces = corners.size();

    // calculating the face centers does not depend on clockwise-ness
    for (size_t cornerIndex = 0; cornerIndex < _numFaces; cornerIndex++)
    {
        glm::vec2 corner1 = corners[cornerIndex];
        glm::vec2 corner2;
        if (cornerIndex + 1 == corners.size())
        {
            // wrap around
            corner2 = corners[0];
        }
        else
        {
            corner2 = corners[cornerIndex + 1];
        }

        // the center if halfway between these two points
        glm::vec2 faceCenter = (corner1 + corner2) * 0.5f;
        _originalFaceCenterPoints[cornerIndex] = faceCenter;
        _currentFaceCenterPoints[cornerIndex] = faceCenter;
    }

    // the normals should point outwards
    // Note: This means that, if the corners are provided clockwise, then each corner->corner
    // vector needs to rotate +90 degrees, but if the corners are provided counterclockwise, 
    // then each corner->corner vector needs to rotate -90 degrees.  Try drawing it out or 
    // working it out with yoru fingers (I did the latter).
    for (size_t cornerIndex = 0; cornerIndex < _numFaces; cornerIndex++)
    {
        glm::vec2 corner1 = corners[cornerIndex];
        glm::vec2 corner2;
        if (cornerIndex + 1 == corners.size())
        {
            // wrap around
            corner2 = corners[0];
        }
        else
        {
            corner2 = corners[cornerIndex + 1];
        }

        glm::vec2 faceNormal = RotateNeg90(corner2 - corner1);
        _originalFaceNormals[cornerIndex] = faceNormal;
        _currentFaceNormals[cornerIndex] = faceNormal;
    }
}

/*-----------------------------------------------------------------------------------------------
Description:
    Checks if the provided particle has gone outside of the polygonal boundaries.
Parameters:
    p   A const reference to a particle object.
Returns:
    True if the particle has outside of the region's boundaries, otherwise false.
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
bool ParticleRegionPolygon::OutOfBounds(const Particle &p) const
{
    bool outsidePolygon = false;
    for (size_t faceIndex = 0; faceIndex < _numFaces; faceIndex++)
    {
        // if any of these checks are true, then this flag will be true as well
        glm::vec2 v1 = p._position - _currentFaceCenterPoints[faceIndex];
        glm::vec2 v2 = _currentFaceNormals[faceIndex];
        outsidePolygon |= (glm::dot(v1, v2) > 0);
    }

    return outsidePolygon;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Applies the transform to each face center point and face normal.  Center points are rotated 
    and translated, while normals are only rotated.
Parameters:
    m       A 4x4 transform matrix.  Because glm transform functions only spit out a 4x4.
Returns:    None
Exception:  Safe
Creator:    John Cox (8-26-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleRegionPolygon::SetTransform(const glm::mat4 &m)
{
    for (size_t faceIndex = 0; faceIndex < _numFaces; faceIndex++)
    {
        glm::vec2 endOfNormal = _currentFaceCenterPoints[faceIndex] + _currentFaceNormals[faceIndex];


        // the extra 1.0f at the end means that it can be translated via 4x4 matrix
        glm::vec4 faceCenterAsVec4 = 
            glm::vec4(_originalFaceCenterPoints[faceIndex], 0.0f, 1.0f);
        _currentFaceCenterPoints[faceIndex] = glm::vec2(m * faceCenterAsVec4);

        // normals can be rotated but should never be translated because they are always relative to the face, so make the W value of the vec4 a 0, which will disable translation
        glm::vec4 faceNormalAsVec4 =
            glm::vec4(_originalFaceNormals[faceIndex], 0.0f, 0.0f);
        _currentFaceNormals[faceIndex] = glm::vec2(m * faceNormalAsVec4); 


        // do NOT transform the normals, which must always be relative to the surface
    }
}