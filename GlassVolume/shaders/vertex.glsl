#version 400 core

in vec3 position;
in vec3 normal;
in vec2 texCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 eyePos;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;
out vec3 vray_dir;
flat out vec3 transformed_eye;

void main()
{
    gl_Position = proj * view * model * vec4(position, 1.0);
    TexCoords = texCoords;
    FragPos = vec3(model * vec4(position, 1.0));
    Normal = normal;

	transformed_eye = eyePos;
	vray_dir = position - transformed_eye;
}