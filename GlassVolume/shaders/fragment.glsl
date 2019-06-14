#version 150 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform vec4 objColor;

out vec4 outColor;

void main()
{
	outColor = objColor;
}