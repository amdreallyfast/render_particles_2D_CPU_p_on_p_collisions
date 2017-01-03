#version 440

// position in window space (both X and Y on the range [-1,+1])
layout (location = 0) in vec2 pos;  

// velocity also in window space (ex: an X speed of 1.0 would cross the window horizontally in 2 
// seconds)
layout (location = 1) in vec2 vel;  


// must have the same name as its corresponding "in" item in the frag shader
smooth out vec3 particleColor;

void main()
{
    float red = 0.0f;       // high velocity
    float green = 0.0f;     // medium velocity
    float blue = 0.0f;      // low velocity

    float velocity = length(vel);
    float mid = 0.3f;
    float max = 0.5f;
    if (velocity < mid)
    {
        // low - medium velocity => linearly blend blue and green
        float fraction = velocity / mid;

        blue = (1.0f - fraction);
        green = fraction;
    }
    else
    {
        // medium - high velocity => linearly blend green and red
        float fraction = velocity / max;

        green = (1.0f - fraction);
        red = fraction;
    }

    //// hard code a white particle color
    //particleColor = vec3(1.0f, 1.0f, 1.0f);
	
    particleColor = vec3(red, green, blue);
    
    gl_Position = vec4(pos, -1.0f, 1.0f);
}

