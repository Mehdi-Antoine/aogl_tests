#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <vector>

#include <cmath>

#include "glew/glew.h"

#include "GLFW/glfw3.h"
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/imguiRenderGL3.h"

#include "glm/glm.hpp"
#include "glm/vec3.hpp" // glm::vec3
#include "glm/vec4.hpp" // glm::vec4, glm::ivec4
#include "glm/mat4x4.hpp" // glm::mat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "glm/gtc/type_ptr.hpp" // glm::value_ptr

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif

#if DEBUG_PRINT == 0
#define debug_print(FORMAT, ...) ((void)0)
#else
#ifdef _MSC_VER
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif
#endif

// Font buffers
extern const unsigned char DroidSans_ttf[];
extern const unsigned int DroidSans_ttf_len;    

// Shader utils
int check_link_error(GLuint program);
int check_compile_error(GLuint shader, const char ** sourceBuffer);
GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize);
GLuint compile_shader_from_file(GLenum shaderType, const char * fileName);

// OpenGL utils
bool checkError(const char* title);

enum LightType{
    POINT,
    DIRECTIONNAL,
    SPOT
};

struct Light
{
    glm::vec3 _pos;
    int _padding;
    glm::vec3 _color;
    float _intensity;
    float _attenuation;

    Light(glm::vec3 pos = glm::vec3(0,0,0), glm::vec3 color = glm::vec3(1,1,1), float intensity = 1, float attenuation = 2){
        _pos = pos;
        _color = color;
        _intensity = intensity;
        _attenuation = attenuation;
    }
};

struct SpotLight
{
    glm::vec3 _pos;
    int _padding1; //16

    glm::vec3 _color;

    float _intensity; //32

    float _attenuation; 
    int _padding2;
    int _padding3;
    int _padding4; //48

    glm::vec3 _dir; 

    float _angle; //64

    float _falloff; //68

    SpotLight(glm::vec3 pos, glm::vec3 dir, glm::vec3 color, float intensity, float attenuation, float angle, float falloff){
        _pos = pos;
        _dir = dir;
        _color = color;
        _intensity = intensity;
        _attenuation = attenuation;
        _angle = angle;
        _falloff = falloff;
    }
};

struct UniformCamera
{
    glm::vec3 _pos;
    int _padding;
    glm::mat4 _screenToWorld;
    glm::mat4 _viewToWorld;

    UniformCamera(glm::vec3 pos, glm::mat4 screenToWorld, glm::mat4 viewToWorld){
        _pos = pos;
        _screenToWorld = screenToWorld;
        _viewToWorld = viewToWorld;
    }
};

struct Camera
{
    float radius;
    float theta;
    float phi;
    glm::vec3 o;
    glm::vec3 eye;
    glm::vec3 up;
};

void camera_defaults(Camera & c);
void camera_zoom(Camera & c, float factor);
void camera_turn(Camera & c, float phi, float theta);
void camera_trav(Camera & c, float x, float y);

struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;
void init_gui_states(GUIStates & guiStates);


int main( int argc, char **argv )
{
    int width = 1800, height= 900;
    float widthf = (float) width, heightf = (float) height;

    float fps = 0.f;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    int const DPI = 2; // For retina screens only
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    int const DPI = 1;
# endif

    // Open a window and create its OpenGL context
    GLFWwindow * window = glfwCreateWindow(width/DPI, height/DPI, "aogl", 0, 0);
    if( ! window )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }
    glfwMakeContextCurrent(window);

    // Init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
          exit( EXIT_FAILURE );
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode( window, GLFW_STICKY_KEYS, GL_TRUE );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );
    GLenum glerr = GL_NO_ERROR;
    glerr = glGetError();

    if (!imguiRenderGLInit(DroidSans_ttf, DroidSans_ttf_len))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    GLuint vertShaderId[3];
    GLuint fragShaderId[6];
    GLuint programObject[6];

    // -------------------- Shader0 for Geometry, Normals, and so on
    vertShaderId[0] = compile_shader_from_file(GL_VERTEX_SHADER, "shaders/tp2/aogl.vert");
    fragShaderId[0] = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/tp2/aogl.frag");
    GLuint geomShaderId = compile_shader_from_file(GL_GEOMETRY_SHADER, "shaders/tp2/aogl.geom");
    programObject[0] = glCreateProgram();
    glAttachShader(programObject[0], vertShaderId[0]);
    glAttachShader(programObject[0], geomShaderId);
    glAttachShader(programObject[0], fragShaderId[0]);
    glLinkProgram(programObject[0]);
    if (check_link_error(programObject[0]) < 0)
        exit(1);

    // -------------------- Shader1 for Debug Drawing

    vertShaderId[1] = compile_shader_from_file(GL_VERTEX_SHADER, "shaders/tp2/blit.vert");
    fragShaderId[1] = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/tp2/blit.frag");
    programObject[1] = glCreateProgram();
    glAttachShader(programObject[1], vertShaderId[1]);
    glAttachShader(programObject[1], fragShaderId[1]);
    glLinkProgram(programObject[1]);
    if (check_link_error(programObject[1]) < 0)
        exit(1);

    // -------------------- Shader2 for Point Light
    fragShaderId[2] = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/tp2/pointLight.frag");
    programObject[2] = glCreateProgram();
    glAttachShader(programObject[2], vertShaderId[1]);
    glAttachShader(programObject[2], fragShaderId[2]);
    glLinkProgram(programObject[2]);
    if (check_link_error(programObject[2]) < 0)
        exit(1);

    // -------------------- Shader3 for Directionnal Light
    fragShaderId[3] = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/tp2/directionnalLight.frag");
    programObject[3] = glCreateProgram();
    glAttachShader(programObject[3], vertShaderId[1]);
    glAttachShader(programObject[3], fragShaderId[3]);
    glLinkProgram(programObject[3]);
    if (check_link_error(programObject[3]) < 0)
        exit(1);

    // -------------------- Shader4 for Spot Light
    fragShaderId[4] = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/tp2/spotLight.frag");
    programObject[4] = glCreateProgram();
    glAttachShader(programObject[4], vertShaderId[1]);
    glAttachShader(programObject[4], fragShaderId[4]);
    glLinkProgram(programObject[4]);
    if (check_link_error(programObject[4]) < 0)
        exit(1);

    // -------------------- Shader5 for Debug Shapes

    vertShaderId[2] = compile_shader_from_file(GL_VERTEX_SHADER, "shaders/tp2/debug.vert");
    fragShaderId[5] = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/tp2/debug.frag");
    programObject[5] = glCreateProgram();
    glAttachShader(programObject[5], vertShaderId[2]);
    glAttachShader(programObject[5], fragShaderId[5]);
    glLinkProgram(programObject[5]);
    if (check_link_error(programObject[5]) < 0)
        exit(1);
    
    // Viewport 
    glViewport( 0, 0, width, height );

    // Create Vao & vbo -------------------------------------------------------------------------------------------------------------------------------

    GLuint vao[4];
    glGenVertexArrays(4, vao);
 
    GLuint vbo[12];
    glGenBuffers(12, vbo);

    // Create Cube -------------------------------------------------------------------------------------------------------------------------------
    int cube_triangleCount = 12;
    int cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, }; 
 
    // Bind the VAO
    glBindVertexArray(vao[0]);
 
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);
 
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
 
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
 
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);
 
    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    

    // Create Plane -------------------------------------------------------------------------------------------------------------------------------

    int plane_triangleCount = 2;
    float textureLoop = 15;
    int plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 0.f, 0.f, textureLoop, textureLoop, 0.f, textureLoop, textureLoop};
    float plane_vertices[] = {-5.0, 0, 5.0, 5.0, 0, 5.0, -5.0, 0, -5.0, 5.0, 0, -5.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};

    // Bind the VAO
    glBindVertexArray(vao[1]);
 
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);
 
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);
 
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);
 
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);
 
    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);



    // My GL Textures -------------------------------------------------------------------------------------------------------------------------------

    int x;
    int y;
    int comp;
    unsigned char * diffuse = stbi_load("textures/spnza_bricks_a_diff.tga", &x, &y, &comp, 3);
    unsigned char * specular = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 3);

    GLuint texture[2];
    glGenTextures(2, texture);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, specular);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // My Lights -------------------------------------------------------------------------------------------------------------------------------

    float lightAttenuation = 4;
    float lightIntensity = 4;
    float lightAttenuationThreshold = 0.01;

    float pointLightsYOffset = -24.6;

    std::vector<Light> pointLights;

    std::vector<Light> directionnalLights;
    directionnalLights.push_back(Light(glm::vec3(0,-1,0), glm::vec3(1,1,1), 0));

    std::vector<SpotLight> spotLights;
    spotLights.push_back(SpotLight(glm::vec3(sqrt(10000),0,sqrt(10000))*0.5f, glm::vec3(0,-1,0), glm::vec3(1,1,0), 1, 1, 60, 90));
    // spotLights.push_back(SpotLight(glm::vec3(20,1,20), glm::vec3(-1,-1,-1), glm::vec3(0.5,1,1), 10, 1, 60, 90));
    // spotLights.push_back(SpotLight(glm::vec3(-10,1,-10), glm::vec3(1,-1,1), glm::vec3(0.5,1,1), 10, 1, 60, 90));

    // My Uniforms -------------------------------------------------------------------------------------------------------------------------------

    // ---------------------- For Geometry Shading
    GLuint mvpLocation = glGetUniformLocation(programObject[0], "MVP");
    GLuint mvLocation = glGetUniformLocation(programObject[0], "MV");

    GLuint mvInverseLocation = glGetUniformLocation(programObject[1], "MVInverse");

    float t = 0;
    GLuint timeLocation = glGetUniformLocation(programObject[0], "Time");

    float SliderValue = 0.3;
    GLuint sliderLocation = glGetUniformLocation(programObject[0], "Slider");

    float SliderMult = 80;
    GLuint sliderMultLocation = glGetUniformLocation(programObject[0], "SliderMult");   

    float specularPower = 20;
    GLuint specularPowerLocation = glGetUniformLocation(programObject[0], "SpecularPower");

    GLuint diffuseLocation = glGetUniformLocation(programObject[0], "Diffuse");
    glProgramUniform1i(programObject[0], diffuseLocation, 0);

    GLuint specularLocation = glGetUniformLocation(programObject[0], "Specular");
    glProgramUniform1i(programObject[0], specularLocation, 1);

    float instanceNumber = 25000;
    GLuint instanceNumberLocation = glGetUniformLocation(programObject[0], "InstanceNumber");
    glProgramUniform1i(programObject[0], instanceNumberLocation, int(instanceNumber));

    if (!checkError("Uniforms"))
        exit(1);

    // ---------------------- For Light Pass Shading

    for(int i = 2; i < 5; ++i){
        GLuint colorBufferLocation = glGetUniformLocation(programObject[i], "ColorBuffer");
        glProgramUniform1i(programObject[i], colorBufferLocation, 0);

        GLuint normalBufferLocation = glGetUniformLocation(programObject[i], "NormalBuffer");
        glProgramUniform1i(programObject[i], normalBufferLocation, 1);

        GLuint depthBufferLocation = glGetUniformLocation(programObject[i], "DepthBuffer");
        glProgramUniform1i(programObject[i], depthBufferLocation, 2);
    }

    if (!checkError("Uniforms"))
        exit(1);

    // ---------------------- For Debug Shading

    GLuint mvpDebugLocation = glGetUniformLocation(programObject[5], "MVP");

    if (!checkError("Uniforms"))
        exit(1);


    // My FBO -------------------------------------------------------------------------------------------------------------------------------

    // Framebuffer object handle
    GLuint gbufferFbo;
    // Texture handles
    GLuint gbufferTextures[3];
    glGenTextures(3, gbufferTextures);
    // 2 draw buffers for color and normal
    GLuint gbufferDrawBuffers[2];

    // Create color texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create normal texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create Framebuffer Object
    glGenFramebuffers(1, &gbufferFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
    // Initialize DrawBuffers
    gbufferDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    gbufferDrawBuffers[1] = GL_COLOR_ATTACHMENT1;
    glDrawBuffers(2, gbufferDrawBuffers);

    // Attach textures to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbufferTextures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gbufferTextures[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferTextures[2], 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }

    // Back to the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create Quad for FBO -------------------------------------------------------------------------------------------------------------------------------

    int   quad_triangleCount = 2;
    int   quad_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float quad_vertices[] =  {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};

    // Quad
    glBindVertexArray(vao[2]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Create Debug Shape -------------------------------------------------------------------------------------------------------------------------------

    int   debug_triangleCount = 12;
    int   debug_triangleList[] = {0, 1, 2, 3, 4, 5, 6, 7, 8}; 
    float debug_vertices[] =  {
                              0,0,0, // cube coords
                              0,0,0, // cube coords
                              0,0,0, // cube coords
                              0,0,0, // cube coords
                              0,0,0, // cube coords
                              0,0,0, // cube coords
                              0,0,0, // cube coords
                              0,0,0, // cube coords
                              0,0,0 // light position
                             };

    // Quad
    glBindVertexArray(vao[3]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[10]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(debug_triangleList), debug_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(debug_vertices), debug_vertices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Create UBO For Light Structures -------------------------------------------------------------------------------------------------------------------------------

    // Create two ubo for light and camera
    GLuint ubo[2];
    glGenBuffers(2, ubo);

    // LIGHT
    GLuint PointLightUniformIndex = glGetUniformBlockIndex(programObject[2], "Light");
    GLuint DirectionnalLightUniformIndex = glGetUniformBlockIndex(programObject[3], "Light");
    GLuint SpotLightUniformIndex = glGetUniformBlockIndex(programObject[4], "Light");

    GLuint LightBindingPoint = 0;
      
    glUniformBlockBinding(programObject[2], PointLightUniformIndex, LightBindingPoint);
    glUniformBlockBinding(programObject[3], DirectionnalLightUniformIndex, LightBindingPoint);
    glUniformBlockBinding(programObject[4], SpotLightUniformIndex, LightBindingPoint);

    glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SpotLight), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferRange(GL_UNIFORM_BUFFER, LightBindingPoint, ubo[0], 0, sizeof(SpotLight));

    // CAM
    GLuint CamUniformIndex1 = glGetUniformBlockIndex(programObject[2], "Camera");
    GLuint CamUniformIndex2 = glGetUniformBlockIndex(programObject[3], "Camera");
    GLuint CamUniformIndex3 = glGetUniformBlockIndex(programObject[4], "Camera");

    GLuint CameraBindingPoint = 1;
      
    glUniformBlockBinding(programObject[2], CamUniformIndex1, CameraBindingPoint);
    glUniformBlockBinding(programObject[3], CamUniformIndex2, CameraBindingPoint);
    glUniformBlockBinding(programObject[4], CamUniformIndex3, CameraBindingPoint);

    glBindBuffer(GL_UNIFORM_BUFFER, ubo[1]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformCamera), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferRange(GL_UNIFORM_BUFFER, CameraBindingPoint, ubo[1], 0, sizeof(UniformCamera));

    // Viewer Structures ----------------------------------------------------------------------------------------------------------------------
    Camera camera;
    camera_defaults(camera);
    GUIStates guiStates;
    init_gui_states(guiStates);
    camera.o = glm::vec3(sqrt(instanceNumber), 0, sqrt(instanceNumber)) * 0.5f;
    camera_turn(camera, 0.7,0);
    camera_zoom(camera, 0.4);

    // GUI vars -------------------------------------------------------------------------------------------------------------------------------

    int logScroll = 0;
    float deltaX = 0;
    float deltaZ = 20;
    camera.o = glm::vec3(sqrt(instanceNumber), 0, sqrt(instanceNumber))*0.5f + glm::vec3(deltaX, 0, deltaZ);


    //*********************************************************************************************
    //***************************************** MAIN LOOP *****************************************
    //*********************************************************************************************
    do
    {
        t = glfwGetTime();

        // Mouse states
        int leftButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE );

        if( leftButton == GLFW_PRESS )
            guiStates.turnLock = true;
        else
            guiStates.turnLock = false;

        if( rightButton == GLFW_PRESS )
            guiStates.zoomLock = true;
        else
            guiStates.zoomLock = false;

        if( middleButton == GLFW_PRESS )
            guiStates.panLock = true;
        else
            guiStates.panLock = false;

        // Camera movements
        int altPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
        if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
        {
            double x; double y;
            glfwGetCursorPos(window, &x, &y);
            guiStates.lockPositionX = x;
            guiStates.lockPositionY = y;
        }
        if (altPressed == GLFW_PRESS)
        {
            double mousex; double mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            int diffLockPositionX = mousex - guiStates.lockPositionX;
            int diffLockPositionY = mousey - guiStates.lockPositionY;
            if (guiStates.zoomLock)
            {
                float zoomDir = 0.0;
                if (diffLockPositionX > 0)
                    zoomDir = -1.f;
                else if (diffLockPositionX < 0 )
                    zoomDir = 1.f;
                camera_zoom(camera, zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
            }
            else if (guiStates.turnLock)
            {
                camera_turn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                            diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

            }
            else if (guiStates.panLock)
            {
                camera_trav(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                            diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
            }
            guiStates.lockPositionX = mousex;
            guiStates.lockPositionY = mousey;
        }

        float speed = 0.005;
        float travX = speed;
        float travY = 0;

        float turnX = 0.0025 * glm::sin(0.7*t);
        float turnY = speed;

        camera_trav(camera, travX, travY);
        camera_turn(camera, turnX, turnY);

        // Get camera matrices
        glm::mat4 projection = glm::perspective(45.0f, widthf / heightf, 0.1f, 10000.f); 
        glm::mat4 worldToView = glm::lookAt(camera.eye, camera.o, camera.up);
        glm::mat4 objectToWorld;
        glm::mat4 mvp = projection * worldToView * objectToWorld;
        glm::mat4 mv = worldToView * objectToWorld;
        glm::mat4 mvInverse = glm::inverse(mv);

        //****************************************** RENDER *******************************************

        // Default states
        glEnable(GL_DEPTH_TEST);
        // Clear the front buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Select shader
        glUseProgram(programObject[0]);

        //-------------------------------------Upload Uniforms

        glProgramUniformMatrix4fv(programObject[0], mvpLocation, 1, 0, glm::value_ptr(mvp));
        glProgramUniformMatrix4fv(programObject[5], mvpDebugLocation, 1, 0, glm::value_ptr(mvp));
        glProgramUniformMatrix4fv(programObject[0], mvLocation, 1, 0, glm::value_ptr(mv));
        glProgramUniformMatrix4fv(programObject[1], mvInverseLocation, 1, 0, glm::value_ptr(mvInverse));

        // Upload value
        glProgramUniform1f(programObject[0], timeLocation, t);
        glProgramUniform1f(programObject[0], sliderLocation, SliderValue);
        glProgramUniform1f(programObject[0], sliderMultLocation, SliderMult);

        glProgramUniform1f(programObject[0], specularPowerLocation,specularPower);
        glProgramUniform1i(programObject[0], instanceNumberLocation, int(instanceNumber));

        //******************************************************* FIRST PASS

        //-------------------------------------Bind gbuffer

        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
        // Clear the gbuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //-------------------------------------Render Cubes

        glBindVertexArray(vao[0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture[1]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, int(instanceNumber));

        //-------------------------------------Render Plane

//        glProgramUniform1i(programObject[0], instanceNumberLocation, -1);
//
//        glBindVertexArray(vao[1]);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, texture[0]);
//        glActiveTexture(GL_TEXTURE1);
//        glBindTexture(GL_TEXTURE_2D, texture[1]);
//        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
//        glBindTexture(GL_TEXTURE_2D, 0);

        //-------------------------------------Unbind the frambuffer

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //******************************************************* SECOND PASS

        //-------------------------------------Light Draw

        // Set a full screen viewport
        glViewport( 0, 0, width, height );

        // Disable the depth test
        glDisable(GL_DEPTH_TEST);
        // Enable blending
        glEnable(GL_BLEND);
        // Setup additive blending
        glBlendFunc(GL_ONE, GL_ONE);

        // Update Camera pos and screenToWorld matrix to all light shaders
        UniformCamera cam(camera.eye, glm::inverse(mvp), mvInverse);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo[1]);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UniformCamera), &cam);
        glBindBuffer(GL_UNIFORM_BUFFER, 0); 

        //------------------------------------ Point Lights

        // point light shaders
        glUseProgram(programObject[2]);

        // Bind quad vao
        glBindVertexArray(vao[2]);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);

        unsigned int nbLightsByCircle[] = {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78};
        int counterCircle = 0;
        unsigned int nbPointLights = 30;
        float xOffset = glm::sqrt(float(instanceNumber))/2;
        float zOffset = glm::sqrt(float(instanceNumber))/2;

        float rayon = sqrt(xOffset*2 + zOffset*2);


        int cptVisiblePointLight = 0;

        std::vector<Light> lights;

        for(size_t i = 0; i < nbPointLights; ++i){

            Light light(glm::vec3(0,0,0), glm::vec3(1,1,1), lightIntensity, lightAttenuation);

            if( i == nbLightsByCircle[counterCircle] ){
              counterCircle++;
              rayon += 3; 
            } 


            float coeff = rayon * sin(t);
            float w = t + t;

//            coeff = 20;
//            w = 0;

            light._pos = glm::vec3(
                coeff * cos(i+ M_PI /nbPointLights) + xOffset,
                pointLightsYOffset,
                coeff * sin(i+ M_PI /nbPointLights) + zOffset);

            light._color.x = cos(i);
            light._color.y = sin(3*i);
            light._color.y = cos(i*2);


            glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Light), &light);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

            lights.push_back(light);

        }

        //------------------------------------ Debug Shape Drawing


//        float bound = std::pow(1./lightAttenuationThreshold,1./lightAttenuation);
//
//        glUseProgram(programObject[5]);
//        glPointSize(5);
//        glBindVertexArray(vao[3]);
//        for(size_t i = 0; i < nbPointLights; ++i){
//
//            float light_vertices[] =  {
//                                        lights[i]._pos.x - bound, lights[i]._pos.y - bound, lights[i]._pos.z - bound,
//                                        lights[i]._pos.x - bound, lights[i]._pos.y - bound, lights[i]._pos.z + bound,
//                                        lights[i]._pos.x + bound, lights[i]._pos.y - bound, lights[i]._pos.z + bound,
//                                        lights[i]._pos.x + bound, lights[i]._pos.y - bound, lights[i]._pos.z - bound,
//
//                                        lights[i]._pos.x - bound, lights[i]._pos.y + bound, lights[i]._pos.z - bound,
//                                        lights[i]._pos.x - bound, lights[i]._pos.y + bound, lights[i]._pos.z + bound,
//                                        lights[i]._pos.x + bound, lights[i]._pos.y + bound, lights[i]._pos.z + bound,
//                                        lights[i]._pos.x + bound, lights[i]._pos.y + bound, lights[i]._pos.z - bound,
//
//                                        lights[i]._pos.x         , lights[i]._pos.y, lights[i]._pos.z         ,         // light pos
//                                      };
//
//            glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
//            glBufferData(GL_ARRAY_BUFFER, sizeof(light_vertices), light_vertices, GL_STATIC_DRAW);
//
//            glDrawElements(GL_POINTS, 9, GL_UNSIGNED_INT, (void*)0);
//        }

        //------------------------------------ Directionnal Lights

        //directionnal light shaders
        glUseProgram(programObject[3]);

        // Bind quad vao
        glBindVertexArray(vao[2]);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);

        for(size_t i = 0; i < directionnalLights.size(); ++i){

            glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Light), &directionnalLights[i]);
            glBindBuffer(GL_UNIFORM_BUFFER, 0); 

            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        }

//        ------------------------------------ Spot Lights

         // spot light shaders
         glUseProgram(programObject[4]);

         // Bind quad vao
         glBindVertexArray(vao[2]);
        
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
         glActiveTexture(GL_TEXTURE2);
         glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);


        spotLights[0]._pos = camera.eye;

         for(size_t i = 0; i < spotLights.size(); ++i){
             glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
             glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SpotLight), &spotLights[i]);
             glBindBuffer(GL_UNIFORM_BUFFER, 0);

             glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
         }

        // Disable blending
        glDisable(GL_BLEND);


//        //-------------------------------------Debug Draw
//
//        glDisable(GL_DEPTH_TEST);
//        glViewport( 0, 0, width/4, height/4 );
//
//        // Select shader
//        glUseProgram(programObject[1]);
//
//
//        // --------------- Color Buffer
//
//        // Bind quad VAO
//        glBindVertexArray(vao[2]);
//
//        glActiveTexture(GL_TEXTURE0);
//        // Bind gbuffer color texture
//        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
//        // Draw quad
//        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
//
//        // --------------- Normal Buffer
//        glViewport( width/4, 0, width/4, height/4 );
//
//        // Bind quad VAO
//        glBindVertexArray(vao[2]);
//
//        glActiveTexture(GL_TEXTURE0);
//        // Bind gbuffer color texture
//        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
//        // Draw quad
//        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
//
//        // --------------- Depth Buffer
//        glViewport( 2*width/4, 0, width/4, height/4 );
//
//        // Bind quad VAO
//        glBindVertexArray(vao[2]);
//
//        glActiveTexture(GL_TEXTURE0);
//        // Bind gbuffer color texture
//        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
//        // Draw quad
//        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);


        //****************************************** EVENTS *******************************************
#if 1
        // Draw UI
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, width, height);

        unsigned char mbut = 0;
        int mscroll = 0;
        double mousex; double mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        mousex*=DPI;
        mousey*=DPI;
        mousey = height - mousey;

        if( leftButton == GLFW_PRESS )
            mbut |= IMGUI_MBUT_LEFT;

        imguiBeginFrame(mousex, mousey, mbut, mscroll);
        char lineBuffer[512];
        // imguiBeginScrollArea("aogl", width - 210, height - 310, 200, 300, &logScroll);

        float xwidth = 400;
        float ywidth = 800;

//        imguiBeginScrollArea("aogl", width - xwidth - 10, height - ywidth - 10, xwidth, ywidth, &logScroll);
//        sprintf(lineBuffer, "FPS %f", fps);
//        imguiLabel(lineBuffer);
//        imguiSlider("Slider", &SliderValue, 0.0, 1.0, 0.001);
//        imguiSlider("SliderMultiply", &SliderMult, 0.0, 1000.0, 0.1);
//        imguiSlider("InstanceNumber", &instanceNumber, 100, 100000, 1);
//        imguiSlider("Specular Power", &specularPower, 0, 100, 0.1);
//        imguiSlider("Attenuation", &lightAttenuation, 0, 16, 0.1);
//        imguiSlider("Intensity", &lightIntensity, 0, 10, 0.1);
//        imguiSlider("Threshold", &lightAttenuationThreshold, 0, 0.5, 0.0001);
//        imguiSlider("yOffset", &pointLightsYOffset, -50, 50, 0.0001);
//
//        for(size_t i = 0; i < spotLights.size(); ++i){
//            imguiSlider("pos.x", &spotLights[i]._pos.x, -10, 10, 0.001);
//            imguiSlider("pos.y", &spotLights[i]._pos.y, -10, 10, 0.001);
//            imguiSlider("pos.z", &spotLights[i]._pos.z, -10, 10, 0.001);
//
//            imguiSlider("dir.x", &spotLights[i]._dir.x, -1, 0, 0.001);
//            imguiSlider("dir.y", &spotLights[i]._dir.y, -1, 0, 0.001);
//            imguiSlider("dir.z", &spotLights[i]._dir.z, -1, 0, 0.001);
//
//            imguiSlider("angle", &spotLights[i]._angle, 0, 180, 0.001);
//            imguiSlider("falloff", &spotLights[i]._falloff, 0, 180, 0.001);
//            imguiSlider("intensity", &spotLights[i]._intensity, 0, 10, 0.001);
//            imguiSlider("attenuation", &spotLights[i]._attenuation, 0, 10, 0.001);
//        }
//
//        imguiEndScrollArea();
//        imguiEndFrame();
//        imguiRenderGLDraw(width, height);

        glDisable(GL_BLEND);
#endif
        // Check for errors
        checkError("End loop");

        glfwSwapBuffers(window);
        glfwPollEvents();

        double newTime = glfwGetTime();
        fps = 1.f/ (newTime - t);
    } // Check if the ESC key was pressed
    while( glfwGetKey( window, GLFW_KEY_ESCAPE ) != GLFW_PRESS );

    //*********************************************************************************************
    //************************************* MAIN LOOP END *****************************************
    //*********************************************************************************************

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}

// No windows implementation of strsep
char * strsep_custom(char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s; ; ) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    return 0;
}

int check_compile_error(GLuint shader, const char ** sourceBuffer)
{
    // Get error log size and print it eventually
    int logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        char *token, *string;
        string = strdup(sourceBuffer[0]);
        int lc = 0;
        while ((token = strsep_custom(&string, "\n")) != NULL) {
           printf("%3d : %s\n", lc, token);
           ++lc;
        }
        fprintf(stderr, "Compile : %s", log);
        delete[] log;
    }
    // If an error happend quit
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
        return -1;     
    return 0;
}

int check_link_error(GLuint program)
{
    // Get link error log size and print it eventually
    int logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetProgramInfoLog(program, logLength, &logLength, log);
        fprintf(stderr, "Link : %s \n", log);
        delete[] log;
    }
    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);        
    if (status == GL_FALSE)
        return -1;
    return 0;
}


GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize)
{
    GLuint shaderObject = glCreateShader(shaderType);
    const char * sc[1] = { sourceBuffer };
    glShaderSource(shaderObject, 
                   1, 
                   sc,
                   NULL);
    glCompileShader(shaderObject);
    check_compile_error(shaderObject, sc);
    return shaderObject;
}

GLuint compile_shader_from_file(GLenum shaderType, const char * path)
{
    FILE * shaderFileDesc = fopen( path, "rb" );
    if (!shaderFileDesc)
        return 0;
    fseek ( shaderFileDesc , 0 , SEEK_END );
    long fileSize = ftell ( shaderFileDesc );
    rewind ( shaderFileDesc );
    char * buffer = new char[fileSize + 1];
    fread( buffer, 1, fileSize, shaderFileDesc );
    buffer[fileSize] = '\0';
    GLuint shaderObject = compile_shader(shaderType, buffer, fileSize );
    delete[] buffer;
    return shaderObject;
}


bool checkError(const char* title)
{
    int error;
    if((error = glGetError()) != GL_NO_ERROR)
    {
        std::string errorString;
        switch(error)
        {
        case GL_INVALID_ENUM:
            errorString = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorString = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorString = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            errorString = "GL_OUT_OF_MEMORY";
            break;
        default:
            errorString = "UNKNOWN";
            break;
        }
        fprintf(stdout, "OpenGL Error(%s): %s\n", errorString.c_str(), title);
    }
    return error == GL_NO_ERROR;
}

void camera_compute(Camera & c)
{
    c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;   
    c.eye.y = cos(c.phi) * c.radius + c.o.y ;
    c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;   
    c.up = glm::vec3(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
}

void camera_defaults(Camera & c)
{
    c.phi = 3.14/2.f;
    c.theta = 3.14/2.f;
    c.radius = 10.f;
    camera_compute(c);
}

void camera_zoom(Camera & c, float factor)
{
    c.radius += factor * c.radius ;
    if (c.radius < 0.1)
    {
        c.radius = 10.f;
        c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
    }
    camera_compute(c);
}

void camera_turn(Camera & c, float phi, float theta)
{
    c.theta += 1.f * theta;
    c.phi   -= 1.f * phi;
    if (c.phi >= (2 * M_PI) - 0.1 )
        c.phi = 0.00001;
    else if (c.phi <= 0 )
        c.phi = 2 * M_PI - 0.1;
    camera_compute(c);
}

void camera_trav(Camera & c, float x, float y)
{
    glm::vec3 up(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
    glm::vec3 fwd = glm::normalize(c.o - c.eye);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    c.up = glm::normalize(glm::cross(side, fwd));
    c.o[0] += up[0] * y * c.radius * 2;
    c.o[1] += up[1] * y * c.radius * 2;
    c.o[2] += up[2] * y * c.radius * 2;
    c.o[0] -= side[0] * x * c.radius * 2;
    c.o[1] -= side[1] * x * c.radius * 2;
    c.o[2] -= side[2] * x * c.radius * 2;       
    camera_compute(c);
}

void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
}
