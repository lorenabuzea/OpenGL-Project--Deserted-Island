//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"

#include <iostream>
#include "Skybox.hpp"

int glWindowWidth = 800;
int glWindowHeight = 600;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

gps::Camera myCamera(
				glm::vec3(0.0f, 2.0f, 5.5f), 
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.1f;

bool pressedKeys[1024];
bool wireframeMode = false;
float angleY = 0.0f;
GLfloat lightAngle;

gps::Model3D scene;
gps::Model3D windmill_blades;
gps::Model3D screenQuad;

gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;
gps::Shader skyboxShader;
gps::Shader rainShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

//windmill rotation
float windmillAngle = 0.0f;
glm::vec3 windmillRotationAxis = glm::vec3(1.0f, 0.0f, 0.0f); // Adjust axis if needed
glm::vec3 windmillCentre = glm::vec3(0.424335, 3.28489, -2.0577);
float spinWindmill = false;

//rain enable
bool enableRain = false;
std::vector<glm::vec3> rainPositions;
const int numRaindrops = 5000;
GLuint rainPositionBuffer;

//fog enable
bool enableFog = false;

//punctiformLight enable
glm::vec3 lightPosPointLight;
GLuint lightPosLoc;
bool enablePosLight = false;
GLuint enableLoc;
gps::Shader pointLight;
gps::Model3D lightCube;

//camera animation for sceen presentation
std::vector<glm::vec3> cameraPathPositions = {
	glm::vec3(0.0f, 2.0f, 5.5f), // Starting position
	glm::vec3(5.0f, 3.0f, 10.0f), // Intermediate position
	glm::vec3(10.0f, 5.0f, 15.0f), // Intermediate position
	glm::vec3(15.0f, 2.0f, 20.0f)  // Ending position
};

std::vector<glm::vec3> cameraPathTargets = {
	glm::vec3(0.0f, 0.0f, 0.0f), // Starting target
	glm::vec3(5.0f, 0.0f, 5.0f), // Intermediate target
	glm::vec3(10.0f, 0.0f, 10.0f), // Intermediate target
	glm::vec3(15.0f, 0.0f, 15.0f)  // Ending target
};
bool isCameraAnimating = false; // Tracks if the animation is active
size_t currentPathIndex = 0;    // Tracks the current keyframe
float animationSpeed = 0.01f;   // Speed of the animation



gps::SkyBox mySkyBoxDay;
gps::SkyBox mySkyBoxNight;

bool showDepthMap;

GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);

}

void initRain() {
	rainPositions.resize(numRaindrops);

	for (int i = 0; i < numRaindrops; ++i) {
		float x = static_cast<float>(rand() % 200 - 100) / 10.0f; // Random X [-10, 10]
		float y = static_cast<float>(rand() % 100) / 10.0f + 10.0f; // Random Y [10, 20]
		float z = static_cast<float>(rand() % 200 - 100) / 10.0f; // Random Z [-10, 10]
		rainPositions[i] = glm::vec3(x, y, z);
	}
}

void updateRain() {
	for (int i = 0; i < numRaindrops; ++i) {
		rainPositions[i].y -= 0.2f; // Move downward
		if (rainPositions[i].y < 0.0f) {
			// Reset raindrop to a random position above the scene
			rainPositions[i].y = static_cast<float>(rand() % 100) / 10.0f + 10.0f;
			rainPositions[i].x = static_cast<float>(rand() % 200 - 100) / 10.0f;
			rainPositions[i].z = static_cast<float>(rand() % 200 - 100) / 10.0f;
		}
	}
}

void renderRain(gps::Shader shader) {
	shader.useShaderProgram();

	GLuint modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
	GLuint viewLoc = glGetUniformLocation(shader.shaderProgram, "view");
	GLuint projLoc = glGetUniformLocation(shader.shaderProgram, "projection");

	// Pass view and projection matrices
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glm::vec3 cameraPos = myCamera.getCameraPosition();
	glm::vec3 cameraFront = glm::normalize(myCamera.getCameraFront());
	glm::vec3 cameraUp = myCamera.getCameraUp();
	glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));

	for (int i = 0; i < numRaindrops; ++i) {
		glm::vec3 raindropPos = rainPositions[i];
		float windEffect = static_cast<float>(sin(glfwGetTime() * 0.5)) * 0.05f;
		raindropPos.x += windEffect;

		glm::vec3 scale = glm::vec3(0.02f, 0.15f, 0.02f);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, raindropPos);
		model[0] = glm::vec4(cameraRight, 0.0f);
		model[1] = glm::vec4(cameraUp, 0.0f);
		model[2] = glm::vec4(-cameraFront, 0.0f);
		model = glm::scale(model, scale);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		screenQuad.Draw(shader);
	}

	glDisable(GL_BLEND);
}

void setFogParameters(gps::Shader& shader) {
	shader.useShaderProgram();

	// Fog color
	glm::vec3 fogColor(0.7f, 0.7f, 0.7f); // Light gray fog
	glUniform3fv(glGetUniformLocation(shader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));

	// Fog distances
	float fogStart = 10.0f;  // Start applying fog at 10 units
	float fogEnd = 50.0f;    // Fully opaque fog at 50 units
	glUniform1f(glGetUniformLocation(shader.shaderProgram, "fogStart"), fogStart);
	glUniform1f(glGetUniformLocation(shader.shaderProgram, "fogEnd"), fogEnd);

	// Pass the enableFog state
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "enableFog"), enableFog);
}





void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		wireframeMode = !wireframeMode; // Toggle wireframe mode
		if (wireframeMode) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe
		} else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Disable wireframe
		}
	}

	if (key == GLFW_KEY_C && action == GLFW_PRESS)
		spinWindmill = !spinWindmill;

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		enableRain = !enableRain; // Toggle rain
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		enableFog = !enableFog; // Toggle fog
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		enablePosLight = !enablePosLight; // Toggle positional light
	}

	if (key == GLFW_KEY_V && action == GLFW_PRESS) {
		if (isCameraAnimating) {
			isCameraAnimating = false; // Stop the animation
		} else {
			isCameraAnimating = true;  // Start the animation
			currentPathIndex = 0;     // Reset to the first keyframe
		}
	}

	// Handle other keys
	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void updateCameraAnimation(float deltaTime) {
    if (!isCameraAnimating) return;

    static float animationTimer = 0.0f; // Timer for the animation
    const float animationDuration = 12.0f; // Total duration of the animation in seconds

    // Easing function for smooth transitions
    auto easeInOutCubic = [](float t) {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    };

    // Animation parameters
    glm::vec3 orbitCenter = glm::vec3(3.069f, 0.21717f, -1.1429f); // Center of the scene
    float orbitRadius = 8.0f; // Adjusted radius for closer movement
    float lateralOffset = 4.0f; // Reduced lateral offset
    glm::vec3 initialCameraPosition = cameraPathPositions[0]; // Starting position of the camera
    glm::vec3 topCameraPosition = glm::vec3(orbitCenter.x + lateralOffset, 10.0f, orbitCenter.z + orbitRadius); // Target position
    glm::vec3 globalUp = glm::vec3(0.0f, 1.0f, 0.0f); // Global up vector

    animationTimer += deltaTime;

    if (animationTimer < animationDuration / 3.0f) {
        // Phase 1: Move the camera diagonally upward
        float t = animationTimer / (animationDuration / 3.0f);
        t = easeInOutCubic(t); // Apply easing

        // Interpolate between the initial position and the diagonal target
        glm::vec3 newPos = glm::mix(initialCameraPosition, topCameraPosition, t);

        // Update the camera's position and target
        myCamera.setCameraPosition(newPos);
        myCamera.setCameraTarget(orbitCenter); // Look at the center of the scene
        myCamera.setCameraUp(globalUp); // Keep the camera's up direction consistent
    } else if (animationTimer < 2.0f * animationDuration / 3.0f) {
        // Phase 2: First rotation around the scene
        float t = (animationTimer - animationDuration / 3.0f) / (animationDuration / 3.0f);
        t = easeInOutCubic(t); // Apply easing
        float angle = glm::radians(180.0f) * t; // Half-circle rotation

        // Calculate the new position on the circular path
        float x = orbitCenter.x + orbitRadius * cos(angle);
        float z = orbitCenter.z + orbitRadius * sin(angle);
        glm::vec3 newPos = glm::vec3(x, topCameraPosition.y - t * 5.0f, z); // Gradually lower the height

        // Update the camera's position and target
        myCamera.setCameraPosition(newPos);
        myCamera.setCameraTarget(orbitCenter); // Look at the center of the scene
        myCamera.setCameraUp(globalUp); // Keep the camera's up direction consistent
    } else if (animationTimer < animationDuration) {
        // Phase 3: Reverse direction and rotate closer
        float t = (animationTimer - 2.0f * animationDuration / 3.0f) / (animationDuration / 3.0f);
        t = easeInOutCubic(t); // Apply easing
        float angle = glm::radians(360.0f) * (1.0f - t); // Full-circle reverse rotation

        // Calculate the new position on the circular path, moving closer
        float x = orbitCenter.x + (orbitRadius * 0.5f) * cos(angle);
        float z = orbitCenter.z + (orbitRadius * 0.5f) * sin(angle);
        glm::vec3 newPos = glm::vec3(x, orbitCenter.y + 2.0f, z); // Close to the center, low height

        // Update the camera's position and target
        myCamera.setCameraPosition(newPos);
        myCamera.setCameraTarget(orbitCenter); // Look at the center of the scene
        myCamera.setCameraUp(globalUp); // Keep the camera's up direction consistent
    } else {
        // End of the animation
        isCameraAnimating = false;
        animationTimer = 0.0f; // Reset the timer
    }
}




bool firstMouse = true;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse)
	{
		firstMouse = false;
		lastX = xpos;
		lastY = ypos;
	}

	float x_offset = xpos - lastX;
	float y_offset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	x_offset *= sensitivity;
	y_offset *= sensitivity;

	yaw += x_offset;
	pitch += y_offset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCamera.rotate(pitch, yaw);
}

void processMovement(){
	if (pressedKeys[GLFW_KEY_Q]) {
		angleY -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angleY += 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);		
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    
    //window scaling for HiDPI displays
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    //for sRBG framebuffer
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    //for antialising
    glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (__APPLE__)
    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	//glEnable(GL_CULL_FACE); // cull face
	//glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {

	scene.LoadModel("scene/scenaMare.obj","scene/");
	windmill_blades.LoadModel("scene/windmill_blades.obj", "scene/");
	screenQuad.LoadModel("objects/quad/quad.obj");
	lightCube.LoadModel("objects/cube/cube.obj");
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	depthMapShader.loadShader("shaders/shadow.vert", "shaders/shadow.frag");
	depthMapShader.useShaderProgram();
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
	rainShader.loadShader("shaders/rain.vert", "shaders/rain.frag");
	rainShader.useShaderProgram();
	pointLight.loadShader("shaders/lightPoint.vert","shaders/lightPoint.frag");
	pointLight.useShaderProgram();
}

void initUniforms() {
    myCustomShader.useShaderProgram();

    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Set the light direction (direction towards the light)
    lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

    // Set point light position
    lightPosPointLight = glm::vec3(1.91606f, 0.452f, -3.28349f); // Position of the point light
    glm::vec3 lightPosEye = glm::vec3(view * glm::vec4(lightPosPointLight, 1.0f));
    lightPosLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos");
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPosEye));

    // Set enablePosLight uniform
    enableLoc = glGetUniformLocation(myCustomShader.shaderProgram, "enablePosLight");
    glUniform1i(enableLoc, enablePosLight ? 1 : 0);

    // Set light color based on enablePosLight
    if (!enablePosLight) {
        lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // White light
    } else {
        lightColor = glm::vec3(1.0f, 0.647f, 0.0f); // Orange light
    }
    lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
	glGenFramebuffers(1, &shadowMapFBO);
	
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,SHADOW_WIDTH,SHADOW_HEIGHT,0,GL_DEPTH_COMPONENT,GL_FLOAT,NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach the texture to the fbo
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightView = glm::lookAt(glm::vec3(lightRotation * glm::vec4(lightDir,0.0f)), glm::vec3(0.0f),glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat nearPlane = 0.1f, farPlane=50.0f;
	glm::mat4 lightProjection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f,nearPlane,farPlane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

	return lightSpaceTrMatrix;
}

void  initSkyboxNight() {
	std::vector<const GLchar*> facesNight;

		facesNight.push_back("skybox/night/purplenebula_rt.tga");
		facesNight.push_back("skybox/night/purplenebula_lf.tga");
		facesNight.push_back("skybox/night/purplenebula_up.tga");
		facesNight.push_back("skybox/night/purplenebula_dn.tga");
		facesNight.push_back("skybox/night/purplenebula_ft.tga");
		facesNight.push_back("skybox/night/purplenebula_bk.tga");
		mySkyBoxNight.Load(facesNight);

}

void initSkyboxDay() {
	std::vector<const GLchar*> facesDay;
	facesDay.push_back("skybox/day/right.jpg");
	facesDay.push_back("skybox/day/left.jpg");
	facesDay.push_back("skybox/day/top.jpg");
	facesDay.push_back("skybox/day/bottom.jpg");
	facesDay.push_back("skybox/day/front.jpg");
	facesDay.push_back("skybox/day/back.jpg");
	mySkyBoxDay.Load(facesDay);
}


void drawObjects(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    // Draw the main scene (non-animated objects)
    model = glm::mat4(1.0f); // Identity matrix for the static scene
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // Send the normal matrix for the static scene if not depth pass
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }




    // Draw the static scene (non-animated parts)
    scene.Draw(shader);

    // Windmill blades animation
    if (spinWindmill) {
        // Increment the windmill angle to animate spinning
        windmillAngle += 1.0f; // Adjust this value to control the speed of rotation
        if (windmillAngle >= 360.0f) {
            windmillAngle -= 360.0f; // Keep angle in the range [0, 360)
        }
    }

    // Transformations for the windmill blades
    glm::mat4 translationToOrigin = glm::translate(glm::mat4(1.0f), -windmillCentre);
    glm::mat4 translationBack = glm::translate(glm::mat4(1.0f), windmillCentre);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(windmillAngle), windmillRotationAxis);

    // Apply the spinning transformation only to the windmill blades
    model = translationBack * rotationMatrix * translationToOrigin;
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // Send the normal matrix for the windmill blades if not depth pass
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    // Draw only the windmill blades with the spinning transformation
    windmill_blades.Draw(shader);

    // Optionally draw the skybox (if applicable)
    if (!depthPass) {
    	if (!enablePosLight) {
    		// Draw the night skybox when enablePosLight is true
    		mySkyBoxDay.Draw(skyboxShader, view, projection);
    	} else {
    		// Draw the day skybox when enablePosLight is false
    		mySkyBoxNight.Draw(skyboxShader, view, projection);
    	}
    }
	//POTI PUNE AICI IN FUNCTIE DE ENABLEPOSLIGHT SA SE SCHIMBE SKYBOX
}




void renderScene() {
    // Set the polygon mode
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Enable solid rendering
    }

	// Update CAMERA ANIMATION
	static float lastFrameTime = glfwGetTime(); // Time of the last frame
	float currentFrameTime = glfwGetTime(); // Time of the current frame
	float deltaTime = currentFrameTime - lastFrameTime; // Time since the last frame
	lastFrameTime = currentFrameTime;

	// Update camera animation
	if (isCameraAnimating) {
		updateCameraAnimation(deltaTime);
	}

    // Depth maps creation pass
    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawObjects(depthMapShader, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Render depth map on screen - toggled with the M key
    if (showDepthMap) {
        glViewport(0, 0, retina_width, retina_height);

        glClear(GL_COLOR_BUFFER_BIT);

        screenQuadShader.useShaderProgram();

        // Bind the depth map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

        glDisable(GL_DEPTH_TEST);
        screenQuad.Draw(screenQuadShader);
        glEnable(GL_DEPTH_TEST);
    } else {
        // Final scene rendering pass (with shadows)
        glViewport(0, 0, retina_width, retina_height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        myCustomShader.useShaderProgram();

        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));


        enableLoc = glGetUniformLocation(myCustomShader.shaderProgram, "enablePosLight");
        glUniform1i(enableLoc, enablePosLight ? 1 : 0);

    	//COLOR CHANGE BASED ON POSITIONAL LIGHT
        if (!enablePosLight) {
            lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // White light
        } else {
            lightColor = glm::vec3(1.0f, 0.647f, 0.0f); // Orange light
        }
        lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

        // Bind the shadow map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

        glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

        drawObjects(myCustomShader, false);

        if (enablePosLight) {
            myCustomShader.useShaderProgram();

            glm::vec3 lightPosEye = glm::vec3(view * glm::vec4(lightPosPointLight, 1.0f));
            glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPosEye));

            pointLight.useShaderProgram();
            glUniformMatrix4fv(glGetUniformLocation(pointLight.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(pointLight.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            model = glm::mat4(1.0f); // Initialize model as an identity matrix
            model = glm::translate(model, lightPosPointLight); // Move the cube to the desired position
            model = glm::scale(model, glm::vec3(0.03f, 0.03f, 0.03f)); // Scale the cube to make it smaller
            glUniformMatrix4fv(glGetUniformLocation(pointLight.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            //lightCube.Draw(pointLight);
        }
    }

    setFogParameters(myCustomShader); // Fog
    if (enableRain) {
        updateRain();
        renderRain(rainShader); // Add rain shader
    }
}



void cleanup() {
	glDeleteTextures(1,& depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char * argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initSkyboxDay();
	initSkyboxNight();
	initUniforms();
	initFBO();
	initRain();


	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();
		renderScene();		

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();

	return 0;
}
