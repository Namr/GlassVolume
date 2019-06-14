#version 150 core

in vec3 position;
in vec3 normal;
in vec2 texCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

void main()
{
    gl_Position = proj * view * model * vec4(position, 1.0);
    TexCoords = texCoords;
    FragPos = vec3(model * vec4(position, 1.0));
    Normal = normal;
}