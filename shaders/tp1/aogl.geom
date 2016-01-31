#version 410 core

#define M_PI 3.1415926535897932384626433832795

precision highp float;
precision highp int;
layout(std140, column_major) uniform;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform int InstanceNumber;
uniform float Slider;
uniform float SliderMult;
uniform float Frequency;
uniform float Speed;

in block
{
    vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
} In[]; 

out block
{
    vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
}Out;

uniform mat4 MVP;
uniform float Viscosity;
uniform float Curve;
uniform float Time;
uniform float SimulationTime;
float newTime = mod(Time,SimulationTime);

out float maxIntensity;

float intensity = Slider*SliderMult;

vec3 center = vec3(sqrt(InstanceNumber), 0, sqrt(InstanceNumber)) * 0.5;
float maxDist = distance(center, vec3(0, 0, 0));

vec3 newPositions[3];

vec3 computeNewHeight(vec3 pos){
    vec3 result = pos;

    float dst = distance(center, result);
    float scale = (cos((dst/maxDist)*M_PI)/0.5+0.5);
    float newY = intensity * ((cos(2*M_PI*(dst/maxDist)*Frequency-newTime*Speed)/(1+pow(dst,0.7))) / (1+pow(newTime,Viscosity)));
    result.y = newY+Curve*scale+result.y;

    return result;
}

void main()
{   
    for(int i = 0; i < gl_in.length; ++i){
        newPositions[i] = computeNewHeight(In[i].Position);
    }

    vec3 v0 = newPositions[1] - newPositions[0];
    vec3 v1 = newPositions[2] - newPositions[0];
    vec3 newNormal = normalize(cross(v0,v1));

    for(int i = 0; i < gl_in.length; ++i){
        gl_Position = MVP*vec4(newPositions[i],1);
        Out.TexCoord = In[i].TexCoord;
        Out.Position = newPositions[i];
        Out.Normal = newNormal;
        EmitVertex();
    }

    maxIntensity = intensity;
}

