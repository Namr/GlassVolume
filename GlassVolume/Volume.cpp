#include "pch.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "Volume.h"


Volume::Volume(TIFFTexture *tex)
{
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));

	//
	//Use tinyObj to load model data
	//
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str()))
	{
		throw std::runtime_error(err);
	}

	for (const auto &shape : shapes)
	{
		for (const auto &index : shape.mesh.indices)
		{
			//add vertices
			vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
			vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
			vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

			//add normals
			vertices.push_back(attrib.normals[3 * index.normal_index + 0]);
			vertices.push_back(attrib.normals[3 * index.normal_index + 1]);
			vertices.push_back(attrib.normals[3 * index.normal_index + 2]);

			//add blank texture coordinates
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);

			//add triangles
			triangles.push_back(triangles.size());
			triangles.push_back(triangles.size());
			triangles.push_back(triangles.size());
		}
	}

	// generate and bind the buffers assosiated with this chunk in order to assign
	// vertices and color to the mesh
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// load the shaders from their corresponding files
	GLuint vertexShader = loadShader("shaders/vertex.glsl", GL_VERTEX_SHADER);

	GLuint fragmentShader = 0;
	fragmentShader = loadShader("shaders/fragment.glsl", GL_FRAGMENT_SHADER);

	// compile the GPU programs
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	// catch any errors
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "Shader Error vertex: " << infoLog << std::endl;

	}

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "Shader Error fragment: " << infoLog << std::endl;
	}

	// create a program from the shaders
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glBindFragDataLocation(shaderProgram, 0, "outColor");

	// finilize the program and use it
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// set the array buffer to contain sections the size of a Vertex struct, and
	// pass a pointer to the vector containing them
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
		&vertices[0], GL_STATIC_DRAW);

	// pass and bind triangle data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		triangles.size() * sizeof(GLuint), &triangles[0],
		GL_STATIC_DRAW);

	// pass vertex positions to shader program
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);

	GLint normAttrib = glGetAttribLocation(shaderProgram, "normal");
	glEnableVertexAttribArray(normAttrib);
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	GLint texcoordsAttrib = glGetAttribLocation(shaderProgram, "texCoords");
	glEnableVertexAttribArray(texcoordsAttrib);
	glVertexAttribPointer(texcoordsAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	uniDims = glGetUniformLocation(shaderProgram, "volume_dims");
	uniTrans = glGetUniformLocation(shaderProgram, "model");
	uniView = glGetUniformLocation(shaderProgram, "view");
	uniProj = glGetUniformLocation(shaderProgram, "proj");
	uniEye = glGetUniformLocation(shaderProgram, "eyePos");
	uniVolumeTexture = glGetUniformLocation(shaderProgram, "volumeTexture");

	//the volume texture should always be 0
	glUniform1i(uniVolumeTexture, 0);
	glUniform3i(uniDims, tex->width, tex->height, tex->depth);
}

void Volume::render(Camera &camera)
{
	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(camera.view));
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(camera.proj));
	glUniform3f(uniEye, camera.position.x, camera.position.y, camera.position.z);

	glDrawElements(GL_TRIANGLES, triangles.size(), GL_UNSIGNED_INT, 0);
}

Volume::~Volume()
{
}


GLuint Volume::loadShader(const char *filepath, GLenum type)
{
	FILE *file = fopen(filepath, "rb");
	if (!file)
	{
		return 0;
	}

	long len;
	if (fseek(file, 0, SEEK_END) != 0 || (len = ftell(file)) == -1L)
	{
		fclose(file);
		return 0;
	}
	rewind(file);

	char *buffer = (char *)malloc(len);
	if (fread(buffer, 1, len, file) != len)
	{
		fclose(file);
		free(buffer);
		return 0;
	}
	fclose(file);

	GLuint shader = glCreateShader(type);
	if (shader == 0)
	{
		free(buffer);
		return 0;
	}

	glShaderSource(shader, 1, (const char *const *)&buffer, (GLint *)&len);
	free(buffer);
	return shader;
}