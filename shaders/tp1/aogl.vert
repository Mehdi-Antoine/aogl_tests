#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0


precision highp float;
precision highp int;

uniform int InstanceNumber;

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec3 Normal;
layout(location = TEXCOORD) in vec2 TexCoord;

in int gl_VertexID;
in int gl_InstanceID;

out block
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
} Out;

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main()
{	
	float xValue = gl_InstanceID % int(sqrt(InstanceNumber));
	float zValue = gl_InstanceID / (sqrt(InstanceNumber));

	vec3 pos = vec3(xValue, 0, zValue);
	vec3 worldPos = Position + pos;
	
	Out.TexCoord = TexCoord;
	Out.Normal = Normal;
	Out.Position = worldPos;
}

