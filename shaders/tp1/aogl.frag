#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Specular;
uniform vec3 LightPos;
uniform vec3 CamPos;

layout(location = FRAG_COLOR, index = 0) out vec3 FragColor;

in block
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
} In;

in float maxIntensity;

void main()
{	
	vec3 diffuse = texture(Diffuse, In.TexCoord).rgb;
	vec3 specular = texture(Specular, In.TexCoord).rgb;

	float specularPower = 20;

	vec3 pointToLight = normalize(LightPos - In.Position);

	float ndotl =  clamp(dot(pointToLight, In.Normal), 0.0, 1.0);

	vec3 pointToCam = normalize(CamPos - In.Position);

	vec3 halfVector = normalize(pointToLight.xyz + pointToCam);
	float normalDotHalfVector = clamp(dot(In.Normal, halfVector), 0.0, 1.0);
	vec3 specularColor =  specular * pow(normalDotHalfVector, specularPower);

	vec3 color = diffuse * ndotl * 0.9 + 0.1 * diffuse + specularColor;
	// vec3 color = vec3(In.Position.y/10, 0, 0) * ndotl + 0.1 * vec3(1, 1, 0);

	FragColor = color;
}
