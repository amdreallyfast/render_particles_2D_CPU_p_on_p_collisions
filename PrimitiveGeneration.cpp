#include "PrimitiveGeneration.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glload/include/glload/gl_4_4.h"   // for GLushort
#include "RandomToast.h"
#include <cmath>

/*-----------------------------------------------------------------------------------------------
Description:
    Creates a (in window coordinates) 0.5 wide by 0.5 tall triangle with texture coordinates
    that hit the bottem left, bottom right, and top center of whatever texture will be used with
    it.
Parameters:
    putDataHere     Self-explanatory. 
Returns:    None
Exception:  Safe
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
void GenerateTriangle(GeometryData *putDataHere)
{
    // all points generated in window space
    MyVertex localVerts[] =
    {
        glm::vec2(-0.25f, -0.25f),      // left bottom corner
        glm::vec2(+0.0f, +0.0f),        // texel at bottom left of texture

        glm::vec2(+0.25f, -0.25f),      // right bottom corner
        glm::vec2(+1.0f, +0.0f),        // texel at bottom right of texture

        glm::vec2(+0.0f, +0.25f),       // center top
        glm::vec2(+0.5f, +1.0f),        // texel at top center of texture
    };

    unsigned int numVerts = sizeof(localVerts) / sizeof(MyVertex);
    for (size_t vertCounter = 0; vertCounter < numVerts; vertCounter++)
    {
        MyVertex &v = localVerts[vertCounter];
        putDataHere->_verts.push_back(v);
    }

    // // only three points, so indices are straightforward, going counterclockwise
    GLushort localIndices[] =
    {
        0, 1, 2,
    };

    unsigned int numIndices = sizeof(localIndices) / sizeof(unsigned short);
    for (size_t indicesCounter = 0; indicesCounter < numIndices; indicesCounter++)
    {
        putDataHere->_indices.push_back(localIndices[indicesCounter]);
    }

    putDataHere->_drawStyle = GL_TRIANGLES;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Creates a (in window coordinates) 0.5 wide by 0.5 tall square with texture coordinates that 
    hit the 4 corners of whatever texture will be used with it.
Parameters:
    putDataHere     Self-explanatory.
Returns:    None
Exception:  Safe
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
void GenerateBox(GeometryData *putDataHere)
{
    // all points generated in window space
    MyVertex localVerts[] =
    {
        glm::vec2(-0.25f, -0.25f),      // left bottom corner
        glm::vec2(+0.0f, +0.0f),        // texel at bottom left of texture

        glm::vec2(+0.25f, -0.25f),      // right bottom corner
        glm::vec2(+1.0f, +0.0f),        // texel at bottom right of texture

        glm::vec2(+0.25f, +0.25f),      // right top
        glm::vec2(+1.0f, +1.0f),        // texel at top right of texture

        glm::vec2(-0.25f, +0.25f),      // left top
        glm::vec2(+0.0f, +1.0f),        // texel at top left of texture
    };

    unsigned int numVerts = sizeof(localVerts) / sizeof(MyVertex);
    for (size_t vertCounter = 0; vertCounter < numVerts; vertCounter++)
    {
        MyVertex &v = localVerts[vertCounter];
        putDataHere->_verts.push_back(v);
    }

    // indices going counterclockwise
    GLushort localIndices[] =
    {
        0, 1, 2,
        2, 3, 0,
    };

    unsigned int numIndices = sizeof(localIndices) / sizeof(unsigned short);
    for (size_t indicesCounter = 0; indicesCounter < numIndices; indicesCounter++)
    {
        putDataHere->_indices.push_back(localIndices[indicesCounter]);
    }

    putDataHere->_drawStyle = GL_TRIANGLES;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Creates a 32-point circle with texture coordinates that will, on the 90-degree parts, hit 
    the right center, top center, left center, and bottom center of whatever texture is used with 
    it.

    Note: I could have used sinf(...) and cosf(...) to create the points, but where's the fun in
    that if I have a faster and obtuse algorithm :) ?  Algorithm courtesy of 
    http://slabode.exofire.net/circle_draw.shtml .
Parameters:
    putDataHere     Self-explanatory.
    radius          Values in window coordinates (X and Y on range [-1,+1]).
    wireframeOnly   If true, the circle prepares indices for GL_LINES.  If false, for 
                    GL_TRIANGLES.
Returns:    None
Exception:  Safe
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
void GenerateCircle(GeometryData *putDataHere, float radius, bool wireframeOnly)
{
    // a 32-point, 0.25 radius (window dimensions) circle will suffice for this demo
    unsigned int arcSegments = 32;
    float x = radius;
    float y = 0.0f;
    float textureS = 0.5f;  // texture's "x"
    float textureT = 0.0f;  // texture's "y"
    glm::vec2 textureCenter(0.5f, 0.5f); 
    float theta = 2 * 3.1415926f / float(arcSegments);
    float tangetialFactor = tanf(theta);
    float radialFactor = cosf(theta);
    for (unsigned int segmentCount = 0; segmentCount < 32; segmentCount++)
    {
        MyVertex v;
        v._position = glm::vec2(x, y);

        // without adding the texture's center, the texture coordinates will center on 0, which 
        // will run over into another copy of the texture (not bad, but I want to demonstrate 
        // the use of a texture without unintential duplication)
        v._texturePosition = textureCenter + glm::vec2(textureS, textureT);
        putDataHere->_verts.push_back(v);

        float tx = (-y) * tangetialFactor;
        float ty = x * tangetialFactor;
        float textureTs = (-textureT) * tangetialFactor;    // textureTs = texture tangential S
        float textureTt = textureS * tangetialFactor;

        // add the tangential factor
        x += tx;
        y += ty;
        textureS += textureTs;
        textureT += textureTt;

        // correct using the radial factor
        x *= radialFactor;
        y *= radialFactor;
        textureS *= radialFactor;
        textureT *= radialFactor;
    }

    // make triangles out of the first vertex from the first arc segment, and then the two 
    // vertices from each successive arc segment (draw it out on paper; it makes sense)
    for (unsigned short segmentCount = 0; segmentCount < arcSegments; segmentCount++)
    {
        // wireframe only, then drawing lines and only need to specify the segments' vertices, 
        // but if not wireframe (that is, drawing a solid), then need to specify an origin for 
        // the triangles
        if (!wireframeOnly)
        {
            putDataHere->_indices.push_back(0);
        }

        // always put the starting vertex for this segment
        putDataHere->_indices.push_back(segmentCount);

        // end vertex for the segment might need to circle back around
        if (segmentCount == arcSegments - 1)
        {
            putDataHere->_indices.push_back(0);
        }
        else
        {
            putDataHere->_indices.push_back(segmentCount + 1);
        }
    }


    if (wireframeOnly)
    {
        putDataHere->_drawStyle = GL_LINES;
    }
    else
    {
        putDataHere->_drawStyle = GL_TRIANGLES;
    }
}

/*-----------------------------------------------------------------------------------------------
Description:
    Creates a wireframe polygon out of the coordinates that the user provides.  No scaling is 
    performed.  The user is responsible for giving it the desired shape and size.
Parameters:
    putDataHere     Self-explanatory.
    corners         A const collection of 2D vectors that specify the corners.
    isClockwise     OpenGL defaults to culling clockwise shapes.  Since the corners are provided
                    from outside the function, this option was provided to allow the user to 
                    specify the vertices clockwise and not have them culled.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
void GeneratePolygonWireframe(GeometryData *putDataHere, const std::vector<glm::vec2> &corners,
    bool isClockwise)
{
    // the points are already provided, so put them into a contiguous structure and analyze them 
    // to see how to put them on the texel range of [0,1] 
    std::vector<MyVertex> localVerts;
    for (size_t cornerIndex = 0; cornerIndex < corners.size(); cornerIndex++)
    {
        MyVertex v;
        v._position = corners[cornerIndex];
        localVerts.push_back(v);
    }

    // find a box in which they can fit, and use those for the texture coordinates
    // Note: The texture will likely appear stretched or squished because of this, but this is a
    // demo of functionality.
    float minX = corners[0].x;
    float maxX = corners[0].x;
    float minY = corners[0].y;
    float maxY = corners[0].y;
    for (size_t cornerIndex = 0; cornerIndex < corners.size(); cornerIndex++)
    {
        const glm::vec2 &cornerRef = corners[cornerIndex];
        if (cornerRef.x < minX)
        {
            minX = cornerRef.x;
        }
        else if (cornerRef.x > maxX)
        {
            maxX = cornerRef.x;
        }

        if (cornerRef.y < minY)
        {
            minY = cornerRef.y;
        }
        else if (cornerRef.y > maxY)
        {
            maxY = cornerRef.y;
        }
    }

    // now go through each vertex to see where they fall on the min/max values
    float xRange = maxX - minX;
    float yRange = maxY - minY;
    for (size_t vertexIndex = 0; vertexIndex < localVerts.size(); vertexIndex++)
    {
        MyVertex &vRef = localVerts[vertexIndex];
        vRef._texturePosition.s = (vRef._position.x - minX) / xRange;
        vRef._texturePosition.t = (vRef._position.y - minY) / yRange;
    }

    // finally created the vertices, so now copy them into the geometry structure
    putDataHere->_verts = localVerts;

    // indices are assumed to be from the first corner to the last and back around to the first
    std::vector<GLushort> localIndices;
    if (isClockwise)
    {
        // OpenGL does things counterclockwise, so do the loop backwards
        for (size_t vertexIndex = localVerts.size() - 1; vertexIndex >=0; vertexIndex--)
        {
            // a line needs two points
            localIndices.push_back((unsigned short)vertexIndex);
            if (vertexIndex == 0)
            {
                // loop back around
                localIndices.push_back((unsigned short)(localVerts.size() - 1));
            }
            else
            {
                localIndices.push_back((unsigned short)(vertexIndex - 1));
            }
        }
    }
    else
    {
        for (size_t vertexIndex = 0; vertexIndex < localVerts.size(); vertexIndex++)
        {
            // a line needs two points
            localIndices.push_back((unsigned short)vertexIndex);
            if (vertexIndex + 1 == localVerts.size())
            {
                // loop back around
                localIndices.push_back(0);
            }
            else
            {
                localIndices.push_back((unsigned short)(vertexIndex + 1));
            }
        }
    }

    // copy the indices into the geometry
    putDataHere->_indices = localIndices;

    // this is a wireframe, so don't fill it in
    putDataHere->_drawStyle = GL_LINES;
}

