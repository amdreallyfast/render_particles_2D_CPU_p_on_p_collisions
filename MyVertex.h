#pragma once

#include "glm/vec2.hpp"

/*-----------------------------------------------------------------------------------------------
Description:
    Stores all info necessary to draw a single vertex.
Creator:    John Cox (6-12-2016)
-----------------------------------------------------------------------------------------------*/
struct MyVertex
{
    glm::vec2 _position;

    // not actually used right now, but since I'm still using an old geometry generation scheme, 
    // this has to stick around
    glm::vec2 _texturePosition;
};


