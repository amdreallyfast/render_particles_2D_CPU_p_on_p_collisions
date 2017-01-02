// the OpenGL version include also includes all previous versions
// Build note: Due to a minefield of preprocessor build flags, the gl_load.hpp must come after 
// the version include.
// Build note: Do NOT mistakenly include _int_gl_4_4.h.  That one doesn't define OpenGL stuff 
// first.
// Build note: Also need to link opengl32.lib (unknown directory, but VS seems to know where it 
// is, so don't put in an "Additional Library Directories" entry for it).
// Build note: Also need to link glload/lib/glloadD.lib.
#include "glload/include/glload/gl_4_4.h"
#include "glload/include/glload/gl_load.hpp"

// Build note: Must be included after OpenGL code (in this case, glload).
// Build note: Also need to link freeglut/lib/freeglutD.lib.  However, the linker will try to 
// find "freeglut.lib" (note the lack of "D") instead unless the following preprocessor 
// directives are set either here or in the source-building command line (VS has a
// "Preprocessor" section under "C/C++" for preprocessor definitions).
// Build note: Also need to link winmm.lib (VS seems to know where it is, so don't put in an 
// "Additional Library Directories" entry).
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "freeglut/include/GL/freeglut.h"

// this linking approach is very useful for portable, crude, barebones demo code, but it is 
// better to link through the project building properties
#pragma comment(lib, "glload/lib/glloadD.lib")
#pragma comment(lib, "opengl32.lib")            // needed for glload::LoadFunctions()
#pragma comment(lib, "freeglut/lib/freeglutD.lib")
#ifdef WIN32
#pragma comment(lib, "winmm.lib")               // Windows-specific; freeglut needs it
#endif

// apparently the FreeType lib also needs a companion file, "freetype261d.pdb"
#pragma comment (lib, "freetype-2.6.1/objs/vc2010/Win32/freetype261d.lib")

// for printf(...)
#include <stdio.h>

// for basic OpenGL stuff
#include "OpenGlErrorHandling.h"
#include "ShaderStorage.h"

// for drawing shapes
#include "GeometryData.h"
#include "PrimitiveGeneration.h"

// for particles
#include "ParticleRegionCircle.h"
#include "ParticleRegionPolygon.h"
#include "ParticleEmitterPoint.h"
#include "ParticleEmitterBar.h"
#include "ParticleStorage.h"
#include "ParticleUpdater.h"
#include "ParticleQuadTree.h"

// for moving the shapes around in window space
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// for the frame rate counter
#include "FreeTypeEncapsulated.h"
#include "Stopwatch.h"

Stopwatch gTimer;

FreeTypeEncapsulated gTextAtlases;

// in a bigger program, uniform locations would probably be stored in the same place as the 
// shader programs
GLint gUnifMatrixTransformLoc;

// in a bigger program, geometry data would be stored in some kind of "scene" or in a renderer
// or behind door number 3 so that collision boxes could get at the vertex data
GeometryData gCircleGeometry;
GeometryData gPolygonGeometry;
GeometryData gQuadTreeGeometry;

// in a bigger program, this would somehow be encapsulated and associated with both the circle
// geometry and the circle particle region, and ditto for the polygon
glm::mat4 gRegionTransformMatrix;

// in a bigger program, ??where would particle stuff be stored??
IParticleRegion *gpParticleRegionCircle;
IParticleRegion *gpParticleRegionPolygon;
IParticleEmitter *gpParticleEmitterPoint;
IParticleEmitter *gpParticleEmitterBar;
ParticleUpdater gParticleUpdater;
ParticleQuadTree gParticleQuadTree;


// TODO: change how things are run around here
// - particle storage (just exists)
// - multiple emitter handler (num particles each frame per emitter)
// - particle state updater (pos and "is active")
// - quad tree 
// - particle collider
// - each frame
//  - emit new particles (takes particle collection)
//  - particle position and "is active" update (takes particle collection, particle region, delta time)
//  - region bound check (takes particle collection)
//  - reset quad tree, then generate new (takes particle collection)
//  - collision check gives new velocity (takes particle collection, quad tree)
// TODO: add "num collisions this frame" to each particle, reset each frame in "particle state updater"




// divide between the circle and the polygon regions
// Note: 
// - 10,000 particles => ~60 fps on my computer
// - 15,000 particles => 30-40 fps on my computer
const unsigned int MAX_PARTICLE_COUNT = 15000;
ParticleStorage gParticleStorage;



/*-----------------------------------------------------------------------------------------------
Description:
    Governs window creation, the initial OpenGL configuration (face culling, depth mask, even
    though this is a 2D demo and that stuff won't be of concern), the creation of geometry, and
    the creation of a texture.
Parameters:
    argc    (From main(...)) The number of char * items in argv.  For glut's initialization.
    argv    (From main(...)) A collection of argument strings.  For glut's initialization.
Returns:
    False if something went wrong during initialization, otherwise true;
Exception:  Safe
Creator:    John Cox (3-7-2016)
-----------------------------------------------------------------------------------------------*/
void Init()
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);


    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();

    // FreeType initialization
    shaderStorageRef.NewShader("freetype");
    shaderStorageRef.AddShaderFile("freetype", "shaderTrueType.vert", GL_VERTEX_SHADER);
    shaderStorageRef.AddShaderFile("freetype", "shaderTrueType.frag", GL_FRAGMENT_SHADER);
    shaderStorageRef.LinkShader("freetype");
    GLuint freeTypeProgramId = shaderStorageRef.GetShaderProgram("freetype");
    gTextAtlases.Init("FreeSans.ttf", freeTypeProgramId);

    // generate a circular particle region, point and bar particle emitters, and join them them in a particle updater
    shaderStorageRef.NewShader("particles");
    shaderStorageRef.AddShaderFile("particles", "shaderParticle.vert", GL_VERTEX_SHADER);
    shaderStorageRef.AddShaderFile("particles", "shaderParticle.frag", GL_FRAGMENT_SHADER);
    shaderStorageRef.LinkShader("particles");
    GLuint particleProgramId = shaderStorageRef.GetShaderProgram("particles");

    // the circle starts centered on the origin and the translate matrix will move it
    // Note: The 1.0f makes it translatable.
    gRegionTransformMatrix = glm::translate(glm::mat4(), glm::vec3(+0.3f, +0.3f, 0.0f));
    gRegionTransformMatrix *= glm::rotate(glm::mat4(), 10.0f, glm::vec3(0.0f, 0.0f, 1.0f));

    // circular particle region
    float circleRadius = 0.5f;
    glm::vec2 circleCenter = glm::vec2(0.0f, 0.0f);
    gpParticleRegionCircle = new ParticleRegionCircle(circleCenter, circleRadius);
    gpParticleRegionCircle->SetTransform(gRegionTransformMatrix);

    // polygon particle region
    std::vector<glm::vec2> polygonCorners;
    polygonCorners.push_back(glm::vec2(-0.25f, -0.5f));
    polygonCorners.push_back(glm::vec2(+0.25f, -0.5f));
    polygonCorners.push_back(glm::vec2(+0.5f, +0.25f));
    polygonCorners.push_back(glm::vec2(-0.5f, +0.25f));
    gpParticleRegionPolygon = new ParticleRegionPolygon(polygonCorners);
    gpParticleRegionPolygon->SetTransform(gRegionTransformMatrix);

    // stick the point emitter in the center (changing this would only require some addition/subtraction from the "circle center"
    gpParticleEmitterPoint = new ParticleEmitterPoint(circleCenter, 0.3f, 0.5f);
    gpParticleEmitterPoint->SetTransform(gRegionTransformMatrix);

    // stick the emitter bar on the left side of the circle, have it emit right, and make the 
    // particles slow compared to the point emitter 
    // Note: Have to use vec4s instead of vec2s because glm::translate(...) only spits out mat4.
    glm::vec2 barP1 = glm::vec2(-0.2f, +0.1f);
    glm::vec2 barP2 = glm::vec2(-0.2f, -0.1f);
    glm::vec2 emitDirection(+1.0f, 0.0f);
    float minVel = 0.1f;
    float maxVel = 0.3f;
    gpParticleEmitterBar = new ParticleEmitterBar(barP1, barP2, emitDirection, minVel, maxVel);
    gpParticleEmitterBar->SetTransform(gRegionTransformMatrix);

    // stick the particle region and emitters into a single "updater" object
    gParticleStorage.Init(particleProgramId, MAX_PARTICLE_COUNT);
    //gParticleUpdater.SetRegion(gpParticleRegionCircle);
    gParticleUpdater.SetRegion(gpParticleRegionPolygon);
    gParticleUpdater.AddEmitter(gpParticleEmitterBar, 10);
    gParticleUpdater.AddEmitter(gpParticleEmitterPoint, 10);
    gParticleUpdater.ResetAllParticles(gParticleStorage._allParticles);
    
    // geometry for particle region borders
    shaderStorageRef.NewShader("geometry");
    shaderStorageRef.AddShaderFile("geometry", "shaderGeometry.vert", GL_VERTEX_SHADER);
    shaderStorageRef.AddShaderFile("geometry", "shaderGeometry.frag", GL_FRAGMENT_SHADER);
    shaderStorageRef.LinkShader("geometry");
    GLuint geometryProgramId = shaderStorageRef.GetShaderProgram("geometry");

    gUnifMatrixTransformLoc = shaderStorageRef.GetUniformLocation("geometry", "translateMatrixWindowSpace");
    
    GenerateCircle(&gCircleGeometry, circleRadius, true);
    gCircleGeometry.Init(geometryProgramId);

    GeneratePolygonWireframe(&gPolygonGeometry, polygonCorners, false);
    gPolygonGeometry.Init(geometryProgramId);

    // same radius and center as "circle particle region" 
    // TODO: make this info more general and not so specific to this particlar part of Init();
    glm::vec2 particleRegionCenter = glm::vec2(gRegionTransformMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    float particleRegionRadius = 0.5f;

    gParticleQuadTree.InitializeTree(particleRegionCenter, particleRegionRadius);
    gParticleQuadTree.GenerateGeometry(&gQuadTreeGeometry, true);
    gQuadTreeGeometry.Init(geometryProgramId);


    // the timer will be used for framerate calculations
    gTimer.Init();
    gTimer.Start();
}

// TODO: header
void UpdateAllTheThings()
{
    gParticleUpdater.Update(gParticleStorage._allParticles, 0,
        gParticleStorage._allParticles.size(), 0.01f);

    gParticleQuadTree.ResetTree();
    gParticleQuadTree.AddParticlestoTree(&(gParticleStorage._allParticles));
    gParticleQuadTree.GenerateGeometry(&gQuadTreeGeometry);

    // tell glut to call this display() function again on the next iteration of the main loop
    // Note: https://www.opengl.org/discussion_boards/showthread.php/168717-I-dont-understand-what-glutPostRedisplay()-does
    // Also Note: This display() function will also be registered to run if the window is moved
    // or if the viewport is resized.  If glutPostRedisplay() is not called, then as long as the
    // window stays put and doesn't resize, display() won't be called again (tested with 
    // debugging).
    // Also Also Note: It doesn't matter where this is called in this function.  It sets a flag
    // for glut's main loop and doesn't actually call the registered display function, but I 
    // got into the habbit of calling it at the end.
    glutPostRedisplay();

}

/*-----------------------------------------------------------------------------------------------
Description:
    This is the "update and draw" function.  FreeGlut It tells OpenGL to clear out some color and depth buffers,
    to set up the data to draw, to draw than stuff, and to report any errors that it came across.
    This is not a user-called function.

    This function is registered with glutDisplayFunc(...) during glut's initialization.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void Display()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // update all particle locations and draw them
    //gParticleUpdater.Update(gParticleStorage._allParticles, 0, 
    //    gParticleStorage._allParticles.size(), 0.01f);
    unsigned int numActiveParticles = gParticleUpdater.NumActiveParticles();
    glUseProgram(ShaderStorage::GetInstance().GetShaderProgram("particles"));
    glBindVertexArray(gParticleStorage._vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, gParticleStorage._arrayBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gParticleStorage._sizeBytes, gParticleStorage._allParticles.data());
    glDrawArrays(gParticleStorage._drawStyle, 0, gParticleStorage._allParticles.size());

    // geometry
    // Note: I have read that, due to variances in OpenGL implementation on different drivers, I should avoid extra calls to glUseProgram(...).  On some implementations the extra call might be ignored, on some it reloads the same program, and on others it can crash.  So I am putting the call out in front of the geometry drawing.  
    glUseProgram(ShaderStorage::GetInstance().GetShaderProgram("geometry"));

    // update the quad tree and draw it
    //gParticleQuadTree.ResetTree();
    //gParticleQuadTree.AddParticlestoTree(&(gParticleStorage._allParticles));
    //unsigned int numQuadTreeNodes = gParticleQuadTree.GenerateGeometry(&gQuadTreeGeometry);
    unsigned int numQuadTreeNodes = gParticleQuadTree.NumNodesInUse();
    gQuadTreeGeometry.UpdateBufferData();
    //TODO: the quad tree nodes' locations are based on an already-transformed center point and on particle locations, which don't have a transform, so transition to updating polygon geometry prior to drawing
    glUniformMatrix4fv(gUnifMatrixTransformLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4()));  // identity matrix transform
    glBindVertexArray(gQuadTreeGeometry._vaoId);
    glDrawElements(gQuadTreeGeometry._drawStyle, gQuadTreeGeometry._indices.size(), GL_UNSIGNED_SHORT, 0);

    // draw the particle region borders
    //glUseProgram(ShaderStorage::GetInstance().GetShaderProgram("geometry"));
    glUniformMatrix4fv(gUnifMatrixTransformLoc, 1, GL_FALSE, glm::value_ptr(gRegionTransformMatrix));
    //glBindVertexArray(gCircleGeometry._vaoId);
    //glDrawElements(gCircleGeometry._drawStyle, gCircleGeometry._indices.size(), GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(gPolygonGeometry._vaoId);
    glDrawElements(gPolygonGeometry._drawStyle, gPolygonGeometry._indices.size(), GL_UNSIGNED_SHORT, 0);


    // draw the frame rate once per second in the lower left corner
    // Note: The font textures' orgin is their lower left corner, so the "lower left" in screen 
    // space is just above [-1.0f, -1.0f].
    GLfloat color[4] = { 0.5f, 0.5f, 0.0f, 1.0f };
    char str[32];
    static int elapsedFramesPerSecond = 0;
    static double elapsedTime = 0.0;
    static double frameRate = 0.0;
    elapsedFramesPerSecond++;
    elapsedTime += gTimer.Lap();
    if (elapsedTime > 1.0)
    {
        frameRate = (double)elapsedFramesPerSecond / elapsedTime;
        elapsedFramesPerSecond = 0;
        elapsedTime -= 1.0f;
    }
    sprintf(str, "%.2lf", frameRate);
    float frameRateXY[2] = { -0.99f, -0.99f };
    float scaleXY[2] = { 1.0f, 1.0f };

    // the first time that "get shader program" runs, it will load the atlas
    glUseProgram(ShaderStorage::GetInstance().GetShaderProgram("freetype"));
    gTextAtlases.GetAtlas(48)->RenderText(str, frameRateXY, scaleXY, color);

    // now show number of active particles
    // Note: For some reason, lower case "i" seems to appear too close to the other letters.
    sprintf(str, "active: %d", numActiveParticles);
    float numActiveParticlesXY[2] = { -0.99f, +0.7f };
    gTextAtlases.GetAtlas(48)->RenderText(str, numActiveParticlesXY, scaleXY, color);

    // now draw the number of active quad tree nodes
    sprintf(str, "nodes: %d", numQuadTreeNodes);
    float numActiveNodesXY[2] = { -0.99f, +0.5f };
    gTextAtlases.GetAtlas(48)->RenderText(str, numActiveNodesXY, scaleXY, color);

    // clean up bindings
    glUseProgram(0);
    glBindVertexArray(0);       // unbind this BEFORE the buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // tell the GPU to swap out the displayed buffer with the one that was just rendered
    glutSwapBuffers();

    //// tell glut to call this display() function again on the next iteration of the main loop
    //// Note: https://www.opengl.org/discussion_boards/showthread.php/168717-I-dont-understand-what-glutPostRedisplay()-does
    //// Also Note: This display() function will also be registered to run if the window is moved
    //// or if the viewport is resized.  If glutPostRedisplay() is not called, then as long as the
    //// window stays put and doesn't resize, display() won't be called again (tested with 
    //// debugging).
    //// Also Also Note: It doesn't matter where this is called in this function.  It sets a flag
    //// for glut's main loop and doesn't actually call the registered display function, but I 
    //// got into the habbit of calling it at the end.
    //glutPostRedisplay();
}

/*-----------------------------------------------------------------------------------------------
Description:
    Tell's OpenGL to resize the viewport based on the arguments provided.  This is an 
    opportunity to call glViewport or glScissor to keep up with the change in size.
    
    This is not a user-called function.  It is registered with glutReshapeFunc(...) during 
    glut's initialization.
Parameters:
    w   The width of the window in pixels.
    h   The height of the window in pixels.
Returns:    None
Exception:  Safe
Creator:    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void Reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Executes when the user presses a key on the keyboard.

    This is not a user-called function.  It is registered with glutKeyboardFunc(...) during
    glut's initialization.

    Note: Although the x and y arguments are for the mouse's current position, this function does
    not respond to mouse presses.
Parameters:
    key     The ASCII code of the key that was pressed (ex: ESC key is 27)
    x       The horizontal viewport coordinates of the mouse's current position.
    y       The vertical window coordinates of the mouse's current position
Returns:    None
Exception:  Safe
Creator:    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void Keyboard(unsigned char key, int x, int y)
{
    // this statement is mostly to get ride of an "unreferenced parameter" warning
    printf("keyboard: x = %d, y = %d\n", x, y);
    switch (key)
    {
    case 27:
    {
        // ESC key
        glutLeaveMainLoop();
        return;
    }
    default:
        break;
    }
}

/*-----------------------------------------------------------------------------------------------
Description:
    I don't know what this does, but I've kept it around since early times, and this was the 
    comment given with it:
    
    "Called before FreeGLUT is initialized. It should return the FreeGLUT display mode flags 
    that you want to use. The initial value are the standard ones used by the framework. You can 
    modify it or just return you own set.  This function can also set the width/height of the 
    window. The initial value of these variables is the default, but you can change it."
Parameters:
    displayMode     ??
    width           ??
    height          ??
Returns:
    ??what??
Exception:  Safe
Creator:    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
unsigned int Defaults(unsigned int displayMode, int &width, int &height) 
{
    // this statement is mostly to get ride of an "unreferenced parameter" warning
    printf("Defaults: width = %d, height = %d\n", width, height);
    return displayMode; 
}

/*-----------------------------------------------------------------------------------------------
Description:
    Cleans up GPU memory.  This might happen when the processes die, but be a good memory steward
    and clean up properly.

    Note: A big program would have the textures, program IDs, buffers, and other things 
    encapsulated somewhere, and each other those would do the cleanup, but in this barebones 
    demo, I can just clean up everything here.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void CleanupAll()
{
    // Note: If I attempt to delete an ID that has already been deleted, that is ok.  OpenGL
    // will silently swallow that.
    delete(gpParticleRegionCircle);
    //delete(gpParticleRegionPolygon);
    delete(gpParticleEmitterBar);
    delete(gpParticleEmitterPoint);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Program start and end.
Parameters:
    argc    The number of strings in argv.
    argv    A pointer to an array of null-terminated, C-style strings.
Returns:
    0 if program ended well, which it always does or it crashes outright, so returning 0 is fine
Exception:  Safe
Creator:    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    glutInit(&argc, argv);

    int width = 500;
    int height = 500;
    unsigned int displayMode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
    displayMode = Defaults(displayMode, width, height);

    glutInitDisplayMode(displayMode);
    glutInitContextVersion(4, 4);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    // enable this for automatic message reporting (see OpenGlErrorHandling.cpp)
#define DEBUG
#ifdef DEBUG
    glutInitContextFlags(GLUT_DEBUG);
#endif

    glutInitWindowSize(width, height);
    glutInitWindowPosition(300, 200);
    int window = glutCreateWindow(argv[0]);

    glload::LoadTest glLoadGood = glload::LoadFunctions();
    // ??check return value??

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    if (!glload::IsVersionGEQ(3, 3))
    {
        printf("Your OpenGL version is %i, %i. You must have at least OpenGL 3.3 to run this tutorial.\n",
            glload::GetMajorVersion(), glload::GetMinorVersion());
        glutDestroyWindow(window);
        return 0;
    }

    if (glext_ARB_debug_output)
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB(DebugFunc, (void*)15);
    }

    Init();

    glutIdleFunc(UpdateAllTheThings);
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutMainLoop();

    CleanupAll();

    return 0;
}