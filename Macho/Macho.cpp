/*
 * Macho.cpp
 *
 *  Created on: Jul 28, 2020
 *      Author: Blake Dykes
 */

#include <stdio.h>
#include <iostream>
#include <GL/glew.h>
#include <vector>
#include <GL/glfw3.h>
#include <GL/glfw3native.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include "include/imgui/imgui_internal.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace std;


#define WINDOW_TITLE "Macho"
const float PI = 3.1415926;
const char* GLSL_VERSION = "#version 330";

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif



GLint
	windowWidth = 1600,
	windowHeight = 1200,
	shaderProgram,
	modelLoc,
	viewLoc,
	projLoc,
	lightColorLoc,
	keyLightPosLoc,
	fillLightPosLoc,
	viewPositionLoc,
	ambientLoc,
	keySpecularIntensityLoc,
	keyHighlightSizeLoc;


GLuint
	VBO,
	VAO,
	NBO,
	EBO;

glm::vec3
	cameraPosition = glm::vec3(-7.5f, 0.0f, 7.5f),
	cameraUpY = glm::vec3(0.0f, 1.0f, 0.0f),
	forWardZ = glm::vec3(0.0f, 0.0f, -1.0f),
	front = glm::vec3(1.0f, 0.0f, -1.0f);

int
	indicesLength,
	shiftKeyDown,
	mouseButtonState;

bool firstMouse = true,
	show_demo_window = true,
	show_another_window = false,
	showImGuiWindow = true,
	showImGuiTools = true;

float
	lastMouseX,
	lastMouseY,
	mouseXOffset,
	mouseYOffset,
	yaw = -0.785f,
	pitch = 0.0f,
	rotateSensitivity = 0.000075,
	mooveSpeed = 0.06,
	scrollSpeed = 0.5,
	ambientStrength = 0.3f,
	keySpecularIntensity = 1.25f,
	keyHighlightSize = 0.75f,
	modelXRotation = 0.0f,
	modelYRotation = 0.0f,
	modelZRotation = 0.0f,
	modelXTranslation = 0.0f,
	modelYTranslation = 0.0f,
	modelZTranslation = 0.0f,
	modelXScale = 0.25f,
	modelYScale = 0.25f,
	modelZScale = 0.25f;


//Light color
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

//Light position
glm::vec3 keyLightPosition(-22.326f, 16.744f, -0.93f);

//GLFW window context
GLFWwindow* window;

//Default Lighting for ImGui
const float defaultLighting [9] = {lightColor.x, lightColor.y, lightColor.z,
							ambientStrength, keySpecularIntensity, keyHighlightSize,
							keyLightPosition.x, keyLightPosition.y, keyLightPosition.z};
//Default Model for ImGui
const float defaultModel [9] = { modelXRotation, modelYRotation, modelZRotation,
								modelZTranslation, modelYRotation, modelZTranslation,
								modelXScale, modelYScale, modelZScale};

const glm::vec3 defaultCameraPos = cameraPosition;
const glm::vec3 defaultCameraDir = front;
const glm::vec3 defaultUp = cameraUpY;
const float defaultYaw = yaw;
const float defaultPitch = pitch;



//Forward Function Declarations
void Initialize(int argc, char* argv[]);                                                   //Init GLFW, GLEW, Dear ImGUi
void RenderGraphics(void);                                                                 //Rendering Loop
void CreateShaders(void);                                                                  //Link Vertex and Fragment Shaders With the Shader Program
void CreateBuffers(void);                                                                  //Create Array and Element Array Buffers
void DestroyGraphics(void);                                                                //Exit Cleanup
GLfloat * CalculateNormals(GLfloat vertices [], GLuint indices []);                        //Calculate Trangle Normals
void error_callback(int error, const char* description);                                   //GLFW Error Callback
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);   //GLFW MouseCallBack
void handleKeys(GLFWwindow *window);                                                       //Handle Keyboard Input
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);        //GLFW KeyCallback
void handleMouse(GLFWwindow *window);                                                      //Handle Mouse Input
void CreateImGuiHelp(bool showHelp);                                                       //Create Help Tab
void CreateImGuiTools(bool showTools);                                                     //Create Tools Tab


/*
 * Vertex Shader
 */
const GLchar * vertexShaderSource = GLSL(330,
		layout (location = 0) in vec3 position;
		layout (location = 1) in vec3 color;
		layout (location = 2) in vec3 normals;

		out vec3 mobileColor;
		out vec3 mobileNormals;
		out vec3 FragmentPos;

		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

	void main(void){
		gl_Position = projection * view * model * vec4(position, 1.0f);

		mobileColor = color;

		mobileNormals = mat3(transpose(inverse(model))) * normals;

		FragmentPos = vec3(model * vec4(position, 1.0f));
	}
);

/*
 * Fragment Shader
 */
const GLchar * fragmentShaderSource = GLSL(330,
		in vec3 mobileColor;
		in vec3 mobileNormals;
		in vec3 FragmentPos;

		out vec4 FragColor;

		uniform vec3 lightColor;
		uniform vec3 keyLightPos;
		uniform vec3 viewPosition;
		uniform float keySpecularIntensity;
		uniform float keyHighlightSize;
		uniform float ambientStrength;


	void main(void){

		//Phong lighting model calculations to generate ambient, diffuse, and specular components
		vec3 viewDir = normalize(-FragmentPos);
		vec3 ambient = ambientStrength * lightColor;
		vec3 norm = mobileNormals;

		//Key Light
		//Diffuse
		vec3 keyLightDir = normalize(keyLightPos - FragmentPos); //Calculate the distance from light source to object
		float keyImpact = max(dot(norm, keyLightDir), 0.0);	//Calculate diffuse impact by generating dot product of normal and light
		vec3 keyDiffuse = keyImpact * lightColor; 	//Generate diffuse light

		//Specular
		vec3 keyReflectDir = reflect(-keyLightDir, norm);	//Calculate reflection vector
		float keySpecularComponent = pow(max(dot(viewDir, keyReflectDir), 0.0), keyHighlightSize);
		vec3 keySpecular = keySpecularIntensity * keySpecularComponent * lightColor;

		vec3 result = (ambient + keyDiffuse + keySpecular) * mobileColor;

		//Sends light result to gpu
		FragColor = vec4(result, 1.0f);
	}

);

/*
 * Main Function
 */
int main(int argc, char* argv[])
{
	Initialize(argc, argv);

	while (!glfwWindowShouldClose(window))
	{
		RenderGraphics();
	}
	DestroyGraphics();
	exit(EXIT_SUCCESS);
	return 0;
}

/*
 * Init GLFW, GLEW, Dear ImGUi
 * Call CreateShaders() & CreateBuffers()
 */
void Initialize(int argc, char* argv[])
{

	//Initialize GLFW
	glfwSetErrorCallback(error_callback);
	if(!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	window = glfwCreateWindow(windowWidth, windowHeight, WINDOW_TITLE, NULL, NULL);
	if(!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	//Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum glewInitResult = glewInit();
	if(glewInitResult != GLEW_OK)
	{
		fprintf(stderr, "ERROR: %s\n", glewGetErrorString(glewInitResult));
		exit(EXIT_FAILURE);
	}

	//Initialize Dear ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	//Setup platform/renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(GLSL_VERSION);

	io.Fonts->AddFontDefault();

	//Only Render Triangles if they have a depth less than any triangle at their pixel location
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	CreateShaders();
	CreateBuffers();

	glUseProgram(shaderProgram);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

}

/*
 * Link vertex and fragment shaders with the shader program
 */
void CreateShaders()
{
	shaderProgram = glCreateProgram();

	//Compile Vertex Shader
	GLint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	//Compile Fragment Shader
	GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	//Attach Vertex and Fragment Shaders
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	//Delete the fragment and vertex shaders once attached and linked
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

/*
 * Create vertex and index buffer objects
 */
void CreateBuffers()
{
		GLfloat vertices [] = {

	    // |     Coordinates      |     Vertex Colors	   | # |
		// |   X      Y      Z    |   R       G       B    |   |
			-5.0f, -13.0f, -4.0f, 	0.706f, 0.788f, 0.780f, //0
			-9.0f, -3.0f,  -7.5f, 	0.706f, 0.788f, 0.780f,	//1
			-9.5f, -5.0f,  -13.0f,	0.706f, 0.788f, 0.780f,	//2
			-7.0f,  0.0f,  -13.0f,	0.706f, 0.788f, 0.780f,	//3
			-8.5f,  0.0f,  -10.0f,	0.020f, 0.051f, 0.110f,	//4
			-1.5f,  3.5f,  -13.0f,	0.020f, 0.051f, 0.110f,	//5
			-9.0f,  3.0f,  -8.0f,	0.020f, 0.051f, 0.110f, //6
			-9.0f,  3.5f,  -5.0f,	0.020f, 0.051f, 0.110f,	//7
			-10.0f, 9.0f,  -9.0f,	0.020f, 0.051f, 0.110f,	//8
			-10.5f, 7.0f,  -6.0f,	0.741f, 0.569f, 0.380f,	//9
			-8.0f,  13.0f, -8.0f,	0.020f, 0.051f, 0.110f,	//10
			-11.0f, 13.0f, -6.0f,	0.020f, 0.051f, 0.110f,	//11
			-8.5f,  19.5f, -6.5f, 	0.020f, 0.051f, 0.110f,	//12
			-3.5f,  13.0f, -6.0f,	0.020f, 0.051f, 0.110f,	//13
			-2.0f,  10.0f, -5.0f,	0.020f, 0.051f, 0.110f, //14
			-11.0f, 13.0f, -5.75f,	0.741f, 0.569f, 0.380f,	//15
			-8.5f,  19.5f, -6.25f, 	0.741f, 0.569f, 0.380f,	//16
			-3.5f,  13.0f, -5.75f,	0.741f, 0.569f, 0.380f,	//17
			-2.0f,  10.0f, -4.75f,	0.020f, 0.051f, 0.110f,	//18
			-6.0f,  10.0f, -7.0f,	0.020f, 0.051f, 0.110f,	//19
			-8.0f,  9.0f,  -7.5f,	0.020f, 0.051f, 0.110f,	//20
			-7.0f,  11.0f, -7.0f,	0.169f, 0.114f, 0.114f,	//21
			-10.5f, 8.0f,  -5.5f,	0.020f, 0.051f, 0.110f,	//22
			 0.0f,  10.0f, -5.5f,	0.020f, 0.051f, 0.110f,	//23
			-2.0f,  10.0f, -9.0f,	0.020f, 0.051f, 0.110f,	//24
			-6.0f,  10.0f, -5.5f,	0.020f, 0.051f, 0.110f,	//25
			 5.0f, -13.0f, -4.0f, 	0.706f, 0.788f, 0.780f, //26 - 0
			 9.0f, -3.0f,  -7.5f, 	0.706f, 0.788f, 0.780f,	//27 - 1
			 9.5f, -5.0f,  -13.0f,	0.706f, 0.788f, 0.780f,	//28 - 2
			 7.0f,  0.0f,  -13.0f,	0.706f, 0.788f, 0.780f,	//29 - 3
			 8.5f,  0.0f,  -10.0f,	0.020f, 0.051f, 0.110f,	//30 - 4
			 1.5f,  3.5f,  -13.0f,	0.020f, 0.051f, 0.110f,	//31 - 5
			 9.0f,  3.0f,  -8.0f,	0.020f, 0.051f, 0.110f, //32 - 6
			 9.0f,  3.5f,  -5.0f,	0.020f, 0.051f, 0.110f,	//33 - 7
			 10.0f, 9.0f,  -9.0f,	0.020f, 0.051f, 0.110f,	//34 - 8
			 10.5f, 7.0f,  -6.0f,	0.741f, 0.569f, 0.380f,	//35 - 9
			 8.0f,  13.0f, -8.0f,	0.020f, 0.051f, 0.110f,	//36 - 10
			 11.0f, 13.0f, -6.0f,	0.020f, 0.051f, 0.110f,	//37 - 11
			 8.5f,  19.5f, -6.5f, 	0.020f, 0.051f, 0.110f,	//38 - 12
			 3.5f,  13.0f, -6.0f,	0.020f, 0.051f, 0.110f,	//39 - 13
			 2.0f,  10.0f, -5.0f,	0.020f, 0.051f, 0.110f, //40 - 14
			 11.0f, 13.0f, -5.75f,	0.741f, 0.569f, 0.380f,	//41 - 15
			 8.5f,  19.5f, -6.25f, 	0.741f, 0.569f, 0.380f,	//42 - 16
			 3.5f,  13.0f, -5.75f,	0.741f, 0.569f, 0.380f,	//43 - 17
			 2.0f,  10.0f, -4.75f,	0.020f, 0.051f, 0.110f,	//44 - 18
			 6.0f,  10.0f, -7.0f,	0.020f, 0.051f, 0.110f,	//45 - 19
			 8.0f,  9.0f,  -7.5f,	0.020f, 0.051f, 0.110f,	//46 - 20
			 7.0f,  11.0f, -7.0f,	0.169f, 0.114f, 0.114f,	//47 - 21
			 10.5f, 8.0f,  -5.5f,	0.020f, 0.051f, 0.110f,	//48 - 22
			 0.0f,  10.0f, -5.5f,	0.020f, 0.051f, 0.110f,	//49 - 23
			 2.0f,  10.0f, -9.0f,	0.020f, 0.051f, 0.110f,	//50 - 24
			 6.0f,  10.0f, -5.5f,	0.020f, 0.051f, 0.110f,	//51 - 25
			-8.0f,  7.75f, -0.0f,	0.196f, 0.294f, 0.384f,	//52
			-2.0f,  9.0f,  -3.0f,	0.196f, 0.294f, 0.384f,	//53
			 0.0f,  10.0f, -4.0f, 	0.706f, 0.788f, 0.780f,	//54
			 0.0f,  8.0f,   0.0f,	0.706f, 0.788f, 0.780f, //55
			-0.5f,  8.0f,   0.0f,	0.020f, 0.051f, 0.110f, //56
			-2.0f,  6.0f,   4.0f,	0.741f, 0.569f, 0.380f, //57
			-2.0f,  3.5f,   5.0f,	0.196f, 0.294f, 0.384f,	//58
			-5.5f,  4.0f,   3.5f,	0.020f, 0.051f, 0.110f,	//59
			-6.0f,  2.5f,   3.0f,	0.020f, 0.051f, 0.110f,	//60
			-8.0f,  3.0f,  -1.0f,	0.020f, 0.051f, 0.110f,	//61
			-8.5f, -2.5f,  -3.0f, 	0.243f, 0.333f, 0.384f,	//62
			-5.5f, -9.0f,  -4.0f, 	0.243f, 0.333f, 0.384f,	//63
			-7.0f, -5.75f, -2.0f,	0.741f, 0.569f, 0.380f,	//64
			-4.0f, -6.75f,  1.0f, 	0.741f, 0.569f, 0.380f, //65
			-7.0f,  1.5f,   1.0f,	0.741f, 0.569f, 0.380f,	//66
			-5.5f, -3.9f,   1.5f,	0.741f, 0.569f, 0.380f, //67
			-4.07f,-2.5f,   4.5f,	0.020f, 0.051f, 0.110f,	//68
			-6.85f, 1.75f,  1.25f,	0.020f, 0.051f, 0.110f,	//69
			-3.0f,  0.5f,   6.0f, 	0.243f, 0.333f, 0.384f,	//70
			-5.45f,-3.85f,  1.75f,	0.020f, 0.051f, 0.110f, //71
			-4.1f, -2.53f,  4.4f,	0.020f, 0.051f, 0.110f,	//72
			-2.97f, 0.5f,   6.03f, 	0.020f, 0.051f, 0.110f,	//73
			-2.0f,  1.47f,  6.03f, 	0.020f, 0.051f, 0.110f,	//74
			-2.0f,  1.5f,   6.0f, 	0.020f, 0.051f, 0.110f,	//75
			-2.0f,  3.47f,  5.2f,	0.020f, 0.051f, 0.110f,	//76
			 0.0f,  1.47f,  6.2f,	0.243f, 0.333f, 0.384f, //77
			 0.0f,  4.0f,   5.2f,	0.243f, 0.333f, 0.384f, //78
			-1.5f,  6.0f,   4.2f,	0.243f, 0.333f, 0.384f, //79
			 8.0f,  7.75f, -0.0f,	0.196f, 0.294f, 0.384f,	//80 - 52
			 2.0f,  9.0f,  -3.0f,	0.243f, 0.333f, 0.384f,	//81 - 53
			 0.0f,  10.0f, -4.0f, 	0.706f, 0.788f, 0.780f,	//82 - 54
			 0.0f,  8.0f,   0.0f,	0.706f, 0.788f, 0.780f, //83 - 55
			 0.5f,  8.0f,   0.0f,	0.020f, 0.051f, 0.110f, //84 - 56
			 2.0f,  6.0f,   4.0f,	0.741f, 0.569f, 0.380f, //85 - 57
			 2.0f,  3.5f,   5.0f,	0.196f, 0.294f, 0.384f,	//86 - 58
			 5.5f,  4.0f,   3.5f,	0.020f, 0.051f, 0.110f,	//87 - 59
			 6.0f,  2.5f,   3.0f,	0.020f, 0.051f, 0.110f,	//88 - 60
			 8.0f,  3.0f,  -1.0f,	0.020f, 0.051f, 0.110f,	//89 - 61
			 8.5f, -2.5f,  -3.0f, 	0.243f, 0.333f, 0.384f,	//90 - 62
			 5.5f, -9.0f,  -4.0f, 	0.243f, 0.333f, 0.384f,	//91 - 63
			 7.0f, -5.75f, -2.0f,	0.741f, 0.569f, 0.380f,	//92 - 64
			 4.0f, -6.75f,  1.0f, 	0.741f, 0.569f, 0.380f, //93 - 65
			 7.0f,  1.5f,   1.0f,	0.741f, 0.569f, 0.380f,	//94 - 66
			 5.5f, -3.9f,   1.5f,	0.741f, 0.569f, 0.380f, //95 - 67
			 4.07f,-2.5f,   4.5f,	0.741f, 0.569f, 0.380f,	//96 - 68
			 6.85f, 1.75f,  1.25f,	0.020f, 0.051f, 0.110f,	//97 - 69
			 3.0f,  0.5f,   6.0f, 	0.243f, 0.333f, 0.384f,	//98 - 70
			 5.45f,-3.85f,  1.75f,	0.020f, 0.051f, 0.110f, //99 - 71
			 4.1f, -2.53f,  4.4f,	0.020f, 0.051f, 0.110f,	//100 - 72
			 2.97f, 0.5f,   6.03f, 	0.020f, 0.051f, 0.110f,	//101 - 73
			 2.0f,  1.47f,  6.03f, 	0.020f, 0.051f, 0.110f,	//102 - 74
			 2.0f,  1.5f,   6.0f, 	0.020f, 0.051f, 0.110f,	//103 - 75
			 2.0f,  3.47f,  5.2f,	0.020f, 0.051f, 0.110f,	//104 - 76
			 0.0f,  1.47f,  6.2f,	0.243f, 0.333f, 0.384f, //105 - 77
			 0.0f,  4.0f,   5.2f,	0.243f, 0.333f, 0.384f, //106 - 78
			 1.5f,  6.0f,   4.2f,	0.243f, 0.333f, 0.384f, //107 - 79
			-4.0f, -4.0f,   9.5f,   0.706f, 0.788f, 0.780f,	//108
			-3.0f, -2.0f,  12.0f,	0.706f, 0.788f, 0.780f,	//109
			-2.5f, -1.0f, 11.0f, 	0.706f, 0.788f, 0.780f,	//110
			-0.5f,  1.25f, 9.0f,	0.706f, 0.788f, 0.780f,	//111
			-1.5f,  0.25f, 10.0f,	0.706f, 0.788f, 0.780f,	//112
			-4.5f, -8.0f, 6.5f,		0.169f, 0.114f, 0.114f,	//113
			-3.75f,-5.0f, 12.5f, 	0.169f, 0.114f, 0.114f,	//114
			-3.0f, -3.0f, 14.0f,	0.169f, 0.114f, 0.114f,	//115
			-1.0f,  0.5f, 12.0f,	0.169f, 0.114f, 0.114f,	//116
			-1.5f, -2.0f, 14.0f,	0.020f, 0.051f, 0.110f,	//117
			-3.0f, -1.5f, 14.5f,	0.020f, 0.051f, 0.110f,	//118
			-1.0f, -1.0f, 14.5f,	0.020f, 0.051f, 0.110f,	//119
			-1.0f, -3.0f, 14.0f,	0.020f, 0.051f, 0.110f,	//120
			-2.0f, -3.5f, 14.0f,	0.169f, 0.114f, 0.114f, //121
			-0.75f,-4.0f, 13.5f,	0.169f, 0.114f, 0.114f,	//122
			-0.5f, -8.0f, 6.5f,		0.169f, 0.114f, 0.114f,	//123
			-0.5f, -5.0f, 12.5f, 	0.169f, 0.114f, 0.114f,	//124
			-5.0f, -8.5f, 2.0f, 	0.741f, 0.569f, 0.380f, //125
			-0.5f, -8.5f, 2.0f, 	0.169f, 0.114f, 0.114f, //126
			4.0f,  -4.0f, 9.5f,   	0.706f, 0.788f, 0.780f,	//127 - 108
			3.0f,  -2.0f, 12.0f,	0.706f, 0.788f, 0.780f,	//128 - 109
			2.5f,  -1.0f, 11.0f, 	0.706f, 0.788f, 0.780f,	//129 - 110
			0.5f,   1.25f,9.0f,		0.706f, 0.788f, 0.780f,	//130 - 111
			1.5f,   0.25f,10.0f,	0.706f, 0.788f, 0.780f,	//131 - 112
			4.5f,  -8.0f, 6.5f,		0.169f, 0.114f, 0.114f,	//132 - 113
			3.75f, -5.0f, 12.5f, 	0.169f, 0.114f, 0.114f,	//133 - 114
			3.0f,  -3.0f, 14.0f,	0.169f, 0.114f, 0.114f,	//134 - 115
			1.0f,   0.5f, 12.0f,	0.169f, 0.114f, 0.114f,	//135 - 116
			1.5f,  -2.0f, 14.0f,	0.020f, 0.051f, 0.110f,	//136 - 117
			3.0f,  -1.5f, 14.5f,	0.020f, 0.051f, 0.110f,	//137 - 118
			1.0f,  -1.0f, 14.5f,	0.020f, 0.051f, 0.110f,	//138 - 119
			1.0f,  -3.0f, 14.0f,	0.020f, 0.051f, 0.110f,	//139 - 120
			2.0f,  -3.5f, 14.0f,	0.169f, 0.114f, 0.114f, //140 - 121
			0.75f, -4.0f, 13.5f,	0.169f, 0.114f, 0.114f,	//141 - 122
			0.5f,  -8.0f, 6.5f,		0.169f, 0.114f, 0.114f,	//142 - 123
			0.5f,  -5.0f, 12.5f, 	0.169f, 0.114f, 0.114f,	//143 - 124
			5.0f,  -8.5f, 2.0f, 	0.741f, 0.569f, 0.380f, //144 - 125
			0.5f,  -8.5f, 2.0f, 	0.169f, 0.114f, 0.114f  //145 - 126
	};

		GLuint indices [] = {
			0,   1,		2,	//1
			2,   1,		3,	//2
			1,   4,		3,	//3
			3,   4,		5,	//4
			4,   6,		5,	//5
			1,   6,		4,	//6
			1,   7,		6,	//7
			6,   8,		5,	//8
			7,   9,		6,	//9
			6,   9,		8,	//10
			5,   8,		10,	//11
			9,   11,	8,	//12
			11,  10,	8,	//13
			11,  12,	10,	//14
			10,  12,	13,	//15
			14,  13,	10,	//16
			10,  13,	14,	//17
			14,  12,	13,	//18
			18,  17,	19,	//19
			17,  21,	19,	//20
			19,  21,	20,	//21
			20,  21,	15,	//22
			17,  16,	21,	//23
			21,  16,	15,	//24
			18,  14,	13,	//25
			13,  17,	18,	//26
			17,  13,	12,	//27
			12,  16,	17,	//28
			18,  13,	16,	//29
			18,  17,	16,	//30
			20,  15,	22,	//31
			22,  15,	11,	//32
			22,  15,	9,	//33
			9,   15,	11,	//34
			11,  15,	16,	//35
			16,  12,	11,	//36
			14,  24,	10,	//37
			5,   8,		24,	//38
			8,   10,	24,	//39
			18,  23,	24,	//40
			25,  18,	19,	//41
			25,  19,	20,	//42
			25,  20,	22,	//43
			18,  25,	22,	//44
			26,  28,	27,	//45
			28,  29,	27,	//46
			27,  29,	30,	//47
			29,  31,	30,	//48
			30,  31,	32,	//49
			27,  30,	32,	//50
			27,  32,	33,	//51
			32,  31,	34,	//52
			33,  32,	35,	//53
			32,  34,	35,	//54
			31,  36,	34,	//55
			37,  34,	36,	//57
			37,  36,	38,	//58
			36,  39,	38,	//59
			40,  36,	39,	//60
			36,  40,	39,	//61
			40,  38,	39,	//62
			44,  45,	43,	//63
			43,  45,	47,	//64
			45,  46,	47,	//65
			46,  41,	47,	//66
			35,  34,	37,	//56
			43,  47,	42,	//67
			47,  41,	42,	//68
			44,  39,	40,	//69
			39,  44,	43,	//70
			43,  38,	39,	//71
			38,  43,	42,	//72
			44,  42,	39,	//73
			44,  42,	43,	//74
			46,  48,	41,	//75
			48,  37,	41,	//76
			48,  35,	41,	//77
			35,  37,	41,	//78
			48,  35,	41,	//79
			35,  37,	41,	//80
			37,  42,	41,	//81
			42,  37,	38,	//82
			40,  36,	50,	//83
			31,  50,	34,	//84
			34,  50,	36,	//85
			44,  50,	49,	//86
			51,  45,	44,	//87
			51,  46,	45,	//88
			51,  48,	46,	//89
			44,  48,	51,	//90
			23,  50,	24,	//91
			24,  31,	5,	//92
			31,  24,	50,	//93
			22,  52,	18,	//94
			7,   52,	9,	//95
			52,  22,	9,	//96
			52,  53,	18,	//97
			53,  55,	54,	//98
			18,  53,	54,	//99
			18,  54,	23,	//100
			52,  56,	53,	//101
			56,  53,	55,	//102
			58,  57,	59,	//103
			57,  56,	52,	//104
			60,  59,	61,	//105
			61,  59,	7,	//106
			57,  52,	59,	//107
			59,  52,	7,	//108
			61,  7,		52,	//109
			58,  59,	60,	//110
			62,  1,		63,	//111
			62,  7,		1,	//112
			69,  60,	61,	//113
			69,  61,	66,	//114
			66,  61,	62,	//115
			62,  61,	7,	//116
			64,  67,	62,	//117
			63,  65,	64,	//118
			63,  64,	62,	//119
			68,  71,	67,	//120
			68,  72,	71,	//121
			71,  70,	69,	//122
			67,  71,	69,	//123
			69,  70,	60,	//124
			65,  68,	67,	//125
			67,  66,	62,	//126
			65,  67,	64,	//127
			67,  69,	66,	//128
			71,  72,	70,	//129
			68,  72,	70,	//130
			68,  73,	70,	//131
			70,  74,	73,	//132
			73,  77,	74,	//133
			77,  78,	76,	//134
			74,  77,	75,	//135
			75,  77,	76,	//136
			57,  79,	55,	//137
			57,  76,	79,	//138
			76,  57,	58,	//139
			76,  78,	79,	//140
			70,  74,	60,	//141
			74,  75,	60,	//142
			75,  76,	58,	//143
			75,  58,	60,	//144
			0,   63,	1,	//145
			48,  44,	80,	//146
			33,  35,	80,	//147
			80,  35,	48,	//148
			80,  44,	81,	//149
			81,  82,	83,	//150
			44,  82,	81,	//151
			44,  49,	82,	//152
			80,  81,	84,	//153
			84,  81,	83,	//154
			86,  87,	85,	//155
			85,  80,	84,	//156
			88,  89,	87,	//157
			89,  33,	87,	//158
			85,  87,	80,	//159
			87,  33,	80,	//160
			89,  80,	33,	//161
			86,  88,	87,	//162
			90,  91,	27,	//163
			90,  27,	33,	//164
			97,  89,	88,	//165
			97,  94,	89,	//166
			94,  90,	89,	//167
			90,  33,	89,	//168
			92,  90,	95,	//169
			91,  92,	93,	//170
			91,  90,	92,	//171
			96,  99,	95,	//172
			96,  100,	99,	//173
			99,  97,	98,	//174
			95,  97,	99,	//175
			97,  88,	98,	//176
			93,  95,	96,	//177
			95,  90,	94,	//178
			93,  92,	95,	//179
			95,  94,	97,	//180
			99,  98,	100,//181
			96,  98,	100,//182
			96,  98,	101,//183
			98,  102,	101,//184
			101, 102,	105,//185
			105, 104,	106,//186
			102, 103,	105,//187
			103, 104,	105,//188
			85,  83,	107,//189
			85,  107,	104,//190
			104, 107,	106,//191
			98,  88,	102,//192
			102, 88,	103,//193
			103, 86,	104,//194
			103, 88,	86,	//195
			26,  27,	91,	//196
			104, 86,	85,	//197
			57,  55,	56,	//198
			85,  84,	83,	//199
			79,  78,	107,//200
			79,  107,	55,	//201
			108, 109,	72,	//202-
			109, 110,	72,	//203
			110, 112,	73,	//204
			110, 73,	72,	//205
			112, 111,	73,	//206
			73,  111,	74,	//207
			108, 72,	65,	//208
			111, 77,	74,	//209
			113, 108,	65,	//210
			113, 114,	108,//211
			114, 115,	108,//212
			108, 115,	109,//213
			115, 116,	109,//214
			110, 116,	112,//215
			112, 116,	111,//216
			109, 116,	110,//217
			113, 65,	125,//218
			125, 65,	63,	//219
			117, 118,	119,//220
			115, 117,	118,//221
			115, 118,	116,//222
			118, 119,	116,//223
			115, 120,	118,//224
			118, 120,	119,//225
			121, 122,	120,//226
			121, 120,	115,//227
			121, 115,	114,//228
			121, 114,	122,//229
			114, 124,	123,//230
			114, 123,	113,//231
			113, 123,	126,//232
			113, 125,	126,//233
			114, 124,	122,//234
			126, 125,	63,	//235
			127, 100,	128,//236
			128, 100,	129,//237
			129, 101,	131,//238
			129, 100,	101,//239
			131, 101,	130,//240
			101, 102,	130,//241
			127, 93,	100,//242
			130, 102,	105,//243
			132, 93,	127,//244
			132, 127,	133,//245
			133, 127,	134,//246
			127, 128,	134,//247
			134, 128,	135,//248
			129, 131,	135,//249
			131, 130,	135,//250
			128, 129,	135,//251
			132, 144,	93,	//252
			144, 91,	93,	//253
			136, 137,	138,//254
			134, 137,	136,//255
			134, 135,	137,//256
			137, 135,	138,//257
			134, 137,	139,//258
			137, 138,	139,//259
			140, 139,	141,//260
			140, 134,	139,//261
			140, 133,	134,//262
			140, 141,	133,//263
			133, 142,	143,//264
			133, 132,	142,//265
			132, 142,	145,//266
			132, 145,	144,//267
			133, 141,	143,//268
			145, 91,	144,//269
			111, 130,	77,	//270
			116, 130,	111,//271
			116, 135,	130,//272
			116, 119,	138,//273
			116, 138,	135,//274
			119, 120,	139,//275
			119, 139,	138,//276
			120, 122,	141,//277
			120, 141,	139,//278
			122, 114,	133,//279
			122, 133,	141,//280
			114, 113,	132,//281
			114, 132,	133,//282
			113, 126,	145,//283
			113, 145,	132,//284
	};

	GLfloat * normalArray = CalculateNormals(vertices, indices);

	indicesLength = sizeof(indices)/sizeof(indices[0]);

	//Generate Buffers
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &NBO);

	glBindVertexArray(VAO);

	//Bind Array Buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Bind Element Array Buffer and Assign Pointers 0 - 2
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//Pointer 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Pointer 1
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(sizeof(GLfloat) * 3));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, NBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(&normalArray), &normalArray, GL_STATIC_DRAW);

	//Pointer 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

/*
 * Render Loop
 */
void RenderGraphics(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//Draw Dear ImGui Frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	CreateImGuiHelp(showImGuiWindow);
	CreateImGuiTools(showImGuiTools);

	//Draw GLFW Viewport
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);

	//GLFW Input handlers
	handleKeys(window);
	handleMouse(window);

	glBindVertexArray(VAO);

	//Model Transforms
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(modelXTranslation, modelYTranslation, modelZTranslation));  //Place the object at the center of the viewport
	model = glm::rotate(model, modelYRotation, glm::vec3(0.0f, 1.0f, 0.0f));	                        //Model Y rotation
	model = glm::rotate(model, modelXRotation, glm::vec3(1.0f, 0.0f, 0.0f));	                        //Model X rotation
	model = glm::rotate(model, modelZRotation, glm::vec3(0.0f, 0.0f, 1.0f));	                        //Model Z rotation
	model = glm::scale(model, glm::vec3(modelXScale, modelYScale, modelZScale));	                    //Scales down the object by 25% in the xyz scale

	//View Transforms
	glm::mat4 view;
	view = glm::lookAt(cameraPosition, cameraPosition + front, cameraUpY);

	//Projection Transforms
	glm::mat4 projection;
	projection = glm::perspective(45.0f, (GLfloat)windowWidth / (GLfloat)windowHeight, 0.1f, 100.0f);

	//Send Transforms to Shader Program uniform variables
	modelLoc = glGetUniformLocation(shaderProgram, "model");
	viewLoc = glGetUniformLocation(shaderProgram, "view");
	projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Send Lighting Model Data to Shader Program uniform variables
	lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
	keyLightPosLoc = glGetUniformLocation(shaderProgram, "keyLightPos");
	fillLightPosLoc = glGetUniformLocation(shaderProgram, "fillLightPos");
	viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");
	ambientLoc = glGetUniformLocation(shaderProgram, "ambientStrength");
	keySpecularIntensityLoc = glGetUniformLocation(shaderProgram, "keySpecularIntensity");
	keyHighlightSizeLoc = glGetUniformLocation(shaderProgram, "keyHighlightSize");

	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(keyLightPosLoc, keyLightPosition.x, keyLightPosition.y, keyLightPosition.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
	glUniform1f(ambientLoc, ambientStrength);
	glUniform1f(keySpecularIntensityLoc, keySpecularIntensity);
	glUniform1f(keyHighlightSizeLoc, keyHighlightSize);

	glDrawElements(GL_TRIANGLES, indicesLength, GL_UNSIGNED_INT, 0);

	//Render ImGui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


	glBindVertexArray(0);           //Deactivates the VAO
	glfwSwapBuffers(window);        //Swaps the buffers of the current window if double buffered
	glfwPollEvents();               //Check for input

}

/*
 * Exit Cleanup
 */
void DestroyGraphics(void)
{
	glDeleteProgram(shaderProgram);

	//Buffer Deletion
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &VAO);

	//ImGui Shutdown
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	//GLFW shutdown
	glfwDestroyWindow(window);
	glfwTerminate();
}

GLfloat * CalculateNormals(GLfloat vertices [], GLuint indices []) {

	vector <glm::vec3> normals;
	GLuint numOfTriangles = (sizeof(indices)/sizeof(indices[0])/3);

	//Calculate Normals
	for(GLuint i = 0; i < numOfTriangles*3; i+=3)
	{
		glm::vec3 edge1 = glm::vec3(vertices[indices[i+1]*6], vertices[(indices[i+1]*6) + 1], vertices[(indices[i+1]*6) + 2])
		- glm::vec3(vertices[indices[i]*6], vertices[(indices[i]*6) + 1], vertices[(indices[i]*6) + 2]);

		glm::vec3 edge2 = glm::vec3(vertices[indices[i+2]*6], vertices[(indices[i+2]*6) + 1], vertices[(indices[i+2]*6) + 2])
				- glm::vec3(vertices[indices[i]*6], vertices[(indices[i]*6) + 1], vertices[(indices[i]*6) + 2]);

		glm::vec3 crossProduct = glm::cross(edge1, edge2);
		glm::vec3 normalizedResult = glm::normalize(crossProduct);

		normals.push_back(normalizedResult);
	}

	int normalContainerSize = normals.size();
	GLfloat normArray [normalContainerSize];
	for(int i = 0, z = 0; i < normalContainerSize; i+=3, z++)
	{
		normArray[i] = normals[z].x;
		normArray[i+1] = normals[z].y;
		normArray[i+2] = normals[z].z;
	}

	return normArray;
}

/*
 * GLFWErrorCallback
 */
void error_callback(int error, const char* description)
{
	fprintf(stderr, "error: %s\n", description);
}

/*
 * GLFW MouseButtonCallback
 * Checks for left mouse click
 */
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{

	if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
	{
		mouseButtonState = action;
	}

	if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE)
	{
		mouseButtonState = action;
		firstMouse = true;
	}
}

/*
 * GLFW KeyCallback
 * Checks for shift key press or release
 */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
	{
		shiftKeyDown = action;
	}

	if ((key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) && action == GLFW_RELEASE)
	{
		shiftKeyDown = action;
		firstMouse = true;
	}

}

/*
 * WASD Movement Function
 */
void handleKeys(GLFWwindow *window)
{
	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPosition += mooveSpeed * front;
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPosition -= mooveSpeed * front;
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPosition -= glm::normalize(glm::cross(front, cameraUpY)) * mooveSpeed;
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPosition += glm::normalize(glm::cross(front, cameraUpY)) * mooveSpeed;
}

/*
 * Mouse Rotation Function
 */
void handleMouse(GLFWwindow *window)
{
	double xPos, yPos;
	glfwGetCursorPos(window, &xPos, &yPos);

	if(firstMouse)
	{
		lastMouseX = xPos;
		lastMouseY = yPos;
	}

	mouseXOffset = xPos - lastMouseX;
	mouseYOffset = lastMouseY - yPos;

	mouseXOffset *= rotateSensitivity;
	mouseYOffset *= rotateSensitivity;

	yaw += mouseXOffset;
	pitch += mouseYOffset;

	if(pitch > 89.0)
		pitch = 89.0;
	if(pitch < -89.0)
		pitch = -89.0;

	if(mouseButtonState == GLFW_PRESS && (shiftKeyDown == GLFW_PRESS || shiftKeyDown == GLFW_REPEAT))
	{
		firstMouse = false;
		front.x = cos(yaw);
		front.y = sin(pitch);
		front.z = sin(yaw) * cos(pitch);
	}
}


/*
 * Help Window
 */
void CreateImGuiHelp(bool showHelp)
{

	//	Un-comment to show Dear ImGui Demo Window
	//	if(show_demo_window)
	//		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::Begin("Help");

	//	Un-comment to show Dear ImGui Demo Window
	//	ImGui::Checkbox("Demo Window", &show_demo_window);

	ImGui::AlignTextToFramePadding();
	ImGui::Text("******************************************WELCOME TO MACHO'S HEAD*******************************************" );
	ImGui::Text("************************************************************************************************************" );
	ImGui::Text("**************************************woof***** ^..^      / ************************************************");
	ImGui::Text("******************************************woof> /_/\\_____/ *************************************************");
	ImGui::Text("***********************************************    /\\  /\\ **************************************************");
	ImGui::Text("***********************************************   /  \\/  \\ *************************************************");
	ImGui::Text("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

	ImGui::Spacing();
	ImGui::Text("About the Project: ");
	ImGui::BulletText("This Project was created using the OpenGL API along with the GLEW, GLFW, and GLM C++ extension libraries.");
	ImGui::BulletText("Macho was modeled by manually graphing xyz coordinates for 145 vertices and drawing 284 triangles.");
	ImGui::BulletText("The tools and help interfaces were created by using the Dear IMGUI library.");
	ImGui::BulletText("Thanks for checking this out! Don't forget to play around with the tools");

	ImGui::Spacing();
	ImGui::Text("CONTROLS");
	ImGui::BulletText("W to move forward");
	ImGui::BulletText("A to move left");
	ImGui::BulletText("S to move back");
	ImGui::BulletText("D to move right");
	ImGui::BulletText("Hold Left Shift + Left Mouse Button to rotate");
	ImGui::Spacing();

	//Reset Camera Button
	if(ImGui::Button("Reset Camera"))
			{
				cameraPosition = defaultCameraPos;
				front = defaultCameraDir;
				cameraUpY = defaultUp;
				yaw = defaultYaw;
				pitch = defaultPitch;
			}


	ImGui::End();
}

/*
 * Tool Window
 */
void CreateImGuiTools(bool showTools)
{

	ImGui::Begin("Tools");

	//Lighting Tools
	if(ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_None))
	{
		if(ImGui::Button("Reset Lighting"))
		{
			lightColor = glm::vec3(defaultLighting[0], defaultLighting[1], defaultLighting[2]);
			ambientStrength = defaultLighting[3];
			keySpecularIntensity = defaultLighting[4];
			keyHighlightSize = defaultLighting[5];
			keyLightPosition = glm::vec3(defaultLighting[6], defaultLighting[7], defaultLighting[8]);
		}

		//Light Color Tool
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Light Color - "); ImGui::SameLine();
		ImGui::ColorEdit3("MyColor##3", (float*)&lightColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_Float);
		ImGui::SameLine();
		if(ImGui::Button("Reset Color"))	{lightColor = glm::vec3(defaultLighting[0], defaultLighting[1], defaultLighting[2]);}
		ImGui::Spacing();

		//Light Intensity Tools (Ambient Amount, Specular Intensity and Specular Highlight Size)
		ImGui::Spacing();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Light Intensity"); ImGui::SameLine();
		if(ImGui::Button("Reset Light Intensity"))
		{
			ambientStrength = defaultLighting[3];
			keySpecularIntensity = defaultLighting[4];
			keyHighlightSize = defaultLighting[5];
		}

		//Ambient
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Ambient - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(300);
		ImGui::SliderFloat("##ambient", &ambientStrength, 0, 10); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##ambient2", &ambientStrength);

		//Specular Intensity
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Specular Intensity - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(300);
		ImGui::SliderFloat("##specular", &keySpecularIntensity, -15, 100); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##specular2", &keySpecularIntensity);

		//Specular Highlight Size
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Specular Highlight Size - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(300);
		ImGui::SliderFloat("##highlight", &keyHighlightSize, -50, 50); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##highlight2", &keyHighlightSize);
		ImGui::Spacing();

		//Light Position Editing Tools
		ImGui::Spacing();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Light Position"); ImGui::SameLine();
		if(ImGui::Button("Reset Light Position"))	{keyLightPosition = glm::vec3(defaultLighting[6], defaultLighting[7], defaultLighting[8]);}

		//X Position
		ImGui::AlignTextToFramePadding();
		ImGui::Text("X Position - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##XLight", &keyLightPosition.x, -40, 40); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##XLight2", &keyLightPosition.x);

		//Y Position
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Y Position - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##YLight", &keyLightPosition.y, -40, 40); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##YLight2", &keyLightPosition.y);

		//Z Position
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Z Position - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##ZLight", &keyLightPosition.z, -40, 40); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##ZLight2", &keyLightPosition.z);

	}

	//Model Transformation Tools
	if(ImGui::CollapsingHeader("Model Position", ImGuiTreeNodeFlags_None))
	{

		if(ImGui::Button("Reset Model"))
		{
			modelXRotation = defaultModel[0];
			modelYRotation = defaultModel[1];
			modelZRotation = defaultModel[2];
			modelXTranslation = defaultModel[3];
			modelYTranslation = defaultModel[4];
			modelZTranslation = defaultModel[5];
			modelXScale = defaultModel[6];
			modelYScale = defaultModel[7];
			modelZScale = defaultModel[8];
		}

		//Rotation Tools
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Rotation"); ImGui::SameLine();
		if(ImGui::Button("Reset Rotation"))
		{
			modelXRotation = defaultModel[0];
			modelYRotation = defaultModel[1];
			modelZRotation = defaultModel[2];
		}

		//X Rotation
		ImGui::AlignTextToFramePadding();
		ImGui::Text("X Rotation - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##XRot", &modelXRotation, -6.28, 6.28); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##XRot2", &modelXRotation);

		//Y Rotation
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Y Rotation - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##YRot", &modelYRotation, -6.28, 6.28); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##YRot2", &modelYRotation);

		//Z Rotation
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Z Rotation - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##ZRot", &modelZRotation, -6.28, 6.28); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##ZRot2", &modelZRotation);
		ImGui::Spacing();

		//Translation Tools
		ImGui::Spacing();
		ImGui::Text("Translation"); ImGui::SameLine();
		if(ImGui::Button("Reset Translation"))
		{
			modelXTranslation = defaultModel[3];
			modelYTranslation = defaultModel[4];
			modelZTranslation = defaultModel[5];
		}

		//X Translation
		ImGui::AlignTextToFramePadding();
		ImGui::Text("X Translation - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##XTran", &modelXTranslation, -50, 50); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##XTran2", &modelXTranslation);

		//Y Translation
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Y Translation - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##YTran", &modelYTranslation, -50, 50); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##YTran2", &modelYTranslation);

		//Z Translation
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Z Translation - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##ZTran", &modelZTranslation, -50, 50); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##ZTran2", &modelZTranslation);
		ImGui::Spacing();

		//Scale Tools
		ImGui::Spacing();
		ImGui::Text("Scale"); ImGui::SameLine();
		if(ImGui::Button("Reset Scale"))
		{
			modelXScale = defaultModel[6];
			modelYScale = defaultModel[7];
			modelZScale = defaultModel[8];
		}

		//X Scale
		ImGui::AlignTextToFramePadding();
		ImGui::Text("X Scale - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##XScale", &modelXScale, 0, 5); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##XScale2", &modelXScale);

		//Y Scale
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Y Scale - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##YScale", &modelYScale, 0, 5); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##YScale2", &modelYScale);

		//Z Scale
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Z Scale - "); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("##ZScale", &modelZScale, 0, 5); ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputFloat("##ZScale2", &modelZScale);
	}

	ImGui::End();
}







