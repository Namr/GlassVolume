// GlassVolume.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#define GLEW_STATIC
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtx/quaternion.hpp>

#define HP_LOAD_LIBRARY
#include <holoplay.h>

#include "LeapConnection.h"

#include "Camera.h"
#include "Volume.h"
#include "TIFFTexture.h"

const int SCR_WIDTH = 2560;
const int SCR_HEIGHT = 1600;

GLuint screenFramebuffer;
GLuint renderedTexture;
GLuint depthrenderbuffer;

// getting the looking glass monitor
static GLFWmonitor* getLookingGlassMonitor()
{
	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	GLFWmonitor* lookingGlass = glfwGetPrimaryMonitor();
	bool foundLKG = false;

	// first search for a 2560 x 1600 monitor
	for (int i = 0; i < count; i++)
	{
		const GLFWvidmode* vidMode = glfwGetVideoMode(monitors[i]);
		if (vidMode->width == SCR_WIDTH &&
			vidMode->height == SCR_HEIGHT)
		{
			lookingGlass = monitors[i];
			foundLKG = true;
		}
	}

	// now search by name, which seems to be unreliable
	for (int i = 0; i < count; i++)
	{
		std::string lowerName = glfwGetMonitorName(monitors[i]);
		printf("Found: %s\n", lowerName.c_str());
		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
		if (lowerName.find("looking") != std::string::npos ||
			lowerName.find("lkg") != std::string::npos ||
			lowerName.find("holoplay") != std::string::npos)
		{
			lookingGlass = monitors[i];
			foundLKG = true;
		}
	}

	printf(foundLKG ?
		"found looking glass!\n" :
		"didn't find LKG\n");

	return lookingGlass;
}

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

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Volume Rendering", getLookingGlassMonitor(), nullptr); // Windowed

	glfwMakeContextCurrent(window);

	hp_loadLibrary();
	hp_initialize();
	hp_setupQuiltSettings(0);
	hp_setupQuiltTexture();
	hp_setupBuffers();

	//init OpenGL and link functions in a dynamic way
	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	
	////////////////////////
	//create framebuffers//
	///////////////////////

	// generate frame buffer
	glGenFramebuffers(1, &screenFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, screenFramebuffer);

	// The texture we're going to render to
	glGenTextures(1, &renderedTexture);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	// Poor filtering. Needed !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// The depth buffer
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);
	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

	// check OpenGL errors
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error: " << err << std::endl;
	}

	TIFFTexture volumeData = TIFFTexture("assets/overview.tif");
	Volume volumeObject = Volume(&volumeData);
	//volumeObject.model = glm::scale(volumeObject.model, glm::vec3(2.0, 2.0, 2.0));

	int totalViews = 32;
	Camera cam[32];
	float cameraSize = 0.8f;
	float cameraDistance = -cameraSize / std::tan(glm::radians(14.0f) / 2.0f);
	float fov = glm::radians(14.0f); // field of view
	float aspectRatio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
	float viewCone = glm::radians(40.0f); // 40° in radians

	//camera center
	glm::vec3 focalPosition = glm::vec3(0.0);

	//init all cameras needed
	for (int currentView = 0; currentView < totalViews; currentView++)
	{
		cam[currentView].view = glm::mat4(1.0f);
		cam[currentView].proj = glm::mat4(1.0f);
		cam[currentView].view = glm::translate(cam[currentView].view, focalPosition);
		float offsetAngle = (currentView / (totalViews - 1.0f) - 0.5f) * viewCone;// start at -viewCone * 0.5 and go up to viewCone * 0.5
		// calculate the offset that the camera should move
		float offset = cameraDistance * std::tan(offsetAngle);

		// modify the view matrix (position)
		cam[currentView].view = glm::translate(cam[currentView].view, glm::vec3(offset, 0.0f, cameraDistance));
		
		//The standard model Looking Glass screen is roughly 4.75" vertically. If we assume the average viewing distance for a user sitting at their desk is about 36", our field of view should be about 14°. There is no correct answer, as it all depends on your expected user's distance from the Looking Glass, but we've found the most success using this figure.
										  // fov, aspect ratio, near, far
		cam[currentView].proj = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);
		// modify the projection matrix, relative to the camera size and aspect ratio
		cam[currentView].proj[2][0] += offset / (cameraSize * aspectRatio);
		cam[currentView].position = glm::vec3(cam[currentView].view[3]);
	}

	float lastTime = 0.0f;
	float speed = 3.0f; // 3 units / second
	float pitch = 0.0f;
	float yaw = 0.0f;

	//hand vars
	float lastHandX = 0.0f;
	float lastHandZ = 0.0f;
	float lastHandY = 0.0f;
	float deltaHandX = 0.0f;
	float deltaHandZ = 0.0f;
	float deltaHandY = 0.0f;
	float handSensitvity = 0.2;
	int canMove = 0;
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
				deltaHandY = hand->palm.position.y - lastHandY;
				deltaHandZ = hand->palm.position.z - lastHandZ;
				lastHandX = hand->palm.position.x;
				lastHandY = hand->palm.position.y;
				lastHandZ = hand->palm.position.z;
				canMove = hand->index.is_extended;
			}
			else
			{
				deltaHandX = 0.0f;
				deltaHandY = 0.0f;
				deltaHandZ = 0.0f;
				canMove = 0;
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

		if (canMove)
		{
			yaw += deltaHandX * deltaTime * handSensitvity;
		}
		if (canMove)
		{
			pitch += deltaHandY * deltaTime * handSensitvity;
		}

		glm::quat yawRot = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
		glm::quat pitchRot = glm::angleAxis(pitch, glm::vec3(1, 0, 0));
		volumeObject.model = glm::toMat4(yawRot) * glm::toMat4(pitchRot);

		// Clear the screen to black and set freambuffer
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screenFramebuffer);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_FRAMEBUFFER_SRGB);

		for (int currentView = 0; currentView < totalViews; currentView++)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screenFramebuffer);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			volumeObject.render(cam[currentView]);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindTexture(GL_TEXTURE_2D, renderedTexture);
			hp_copyViewToQuilt(currentView);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		hp_drawLightfield();
	}

	glfwTerminate();

	return 0;
}
