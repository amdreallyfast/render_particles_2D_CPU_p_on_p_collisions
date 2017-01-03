#version 440

// position in window space (both X and Y on the range [-1,+1])
layout (location = 0) in vec2 pos;  

// velocity also in window space (ex: an X speed of 1.0 would cross the window horizontally in 2 
// seconds)
layout (location = 1) in vec2 vel;  

layout (location = 3) in int collisionCountThisFrame;


// must have the same name as its corresponding "in" item in the frag shader
smooth out vec3 particleColor;

void main()
{
    float red = 0.0f;       // high velocity
    float green = 0.0f;     // medium velocity
    float blue = 0.0f;      // low velocity

    // linearly blend the particles based on the minimum and maximum particle velocities
    // TODO: use forces instead

//    float min = 0.1f;
//    float mid = 0.3f;
//    float max = 0.5f;
//
//    float velocity = length(vel);
//    if (velocity < mid)
//    {
//        // low - medium velocity => linearly blend blue and green
//        float fraction = (velocity - min) / (mid - min);
//
//        blue = (1.0f - fraction);
//        green = fraction;
//    }
//    else
//    {
//        // medium - high velocity => linearly blend green and red
//        float fraction = (velocity - mid) / (max - mid);
//
//        green = (1.0f - fraction);
//        red = fraction;
//    }

    float min = 0;
    float mid = 10;
    float max = 20;
    
    float value = collisionCountThisFrame;
    if (value < mid)
    {
        // low - medium velocity => linearly blend blue and green
        float fraction = (value - min) / (mid - min);

        blue = (1.0f - fraction);
        green = fraction;
    }
    else
    {
        // medium - high velocity => linearly blend green and red
        float fraction = (value - mid) / (max - mid);

        green = (1.0f - fraction);
        red = fraction;
    }


    particleColor = vec3(red, green, blue);
    
    gl_Position = vec4(pos, -1.0f, 1.0f);
}

