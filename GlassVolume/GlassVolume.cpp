// GlassVolume.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "LeapConnection.h"

#include "Camera.h"
#include "Volume.h"
#include "TIFFTexture.h"
int main()
{
	//init leap motion connection
	OpenConnection();
	while (!IsConnected)
		millisleep(100); //wait a bit to let the connection complete

	int64_t lastFrameID = 0;

	printf("Connected. ");
	LEAP_DEVICE_INFO* deviceProps = GetDeviceProperties();
	if (deviceProps)
		printf("Using device %s.\n", deviceProps->serial);

	//init window and settings
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(1366, 720, "Electric Field Simulator", nullptr, nullptr); // Windowed

	glfwMakeContextCurrent(window);

	//init OpenGL and link functions in a dynamic way
	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	// check OpenGL errors
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error: " << err << std::endl;
	}

	Camera cam = Camera();
	cam.view = glm::lookAt(
		glm::vec3(10.0f, 10.0f, 10.0f), // position
		glm::vec3(0.0f, 0.0f, 0.0f), // camera center
		glm::vec3(0.0f, 0.0f, 1.0f) // up axis
	);

	TIFFTexture volumeData = TIFFTexture("assets/overview.tif");
	Volume volumeObject = Volume(&volumeData);

	float lastTime = 0.0f;
	float speed = 3.0f; // 3 units / second
	float pitch = 0.0f;
	float yaw = 0.0f;

	//hand vars
	float lastHandX = 0.0f;
	float deltaHandX = 0.0f;
	float handSensitvity = 0.5;

	//main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwSwapBuffers(window);
		glfwPollEvents();

		double currentTime = glfwGetTime();
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		//get leap position
		LEAP_TRACKING_EVENT *frame = GetFrame();
		if (frame && (frame->tracking_frame_id > lastFrameID)) 
		{
			lastFrameID = frame->tracking_frame_id;
			if (frame->nHands > 0)
			{
				//printf("Frame %lli with %i hands.\n", (long long int)frame->tracking_frame_id, frame->nHands);
				LEAP_HAND* hand = &frame->pHands[0];
				deltaHandX = hand->palm.position.x - lastHandX;
				lastHandX = hand->palm.position.x;
			}
			else
			{
				deltaHandX = 0.0f;
			}
		}

		//get inputs
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			pitch += speed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			pitch -= speed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			yaw -= speed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			yaw += speed * deltaTime;
		}

		if (std::abs(deltaHandX) > 0.1)
		{
			yaw += deltaHandX * deltaTime * handSensitvity;
		}

		cam.position.x = 0 + (cos(yaw)  * sin(pitch) * 3);
		cam.position.y = 0 + (sin(yaw) * sin(pitch) * 3);
		cam.position.z = 0 + (cos(pitch) * 3);

		cam.view = glm::lookAt(
			cam.position,             // position
			glm::vec3(0.0f, 0.0f, 00.0f), // camera center
			glm::vec3(0.0f, 0.0f, 1.0f)                    // up axis
		);

		// Clear the screen to black
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		volumeObject.render(cam);

	}

	glfwTerminate();

	return 0;
}
