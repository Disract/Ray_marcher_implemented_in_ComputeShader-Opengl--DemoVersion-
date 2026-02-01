
#include <glad\glad.h>
#include<GLFW/glfw3.h>
#include "./depdencies/include/custom/shader.h"
#include "./depdencies/include/custom/ComputeShader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<math.h>
#include<iostream>


//MAGIC CONSTANTS
const unsigned int magic = 2000;
const unsigned int SCR_WIDTH = magic;
const unsigned int SCR_HEIGHT = magic;
const unsigned int TEXTURE_WIDTH = magic;
const unsigned int TEXTURE_HEIGHT = magic;
float deltaTime = 0.0f; 
float lastFrame = 0.0f;




//MANAGE WINDOW RESIZE
void window_resize_callback(GLFWwindow* window_p, int width, int height)
{
	glViewport(0, 0, width, height);
}

//RENDER A PLAN IN WINDOW
unsigned int quadVBO;
unsigned int quadVAO = 0;
void renderQuad()
{
	//std::cout << "DEBUG3" << std::endl;
	if (quadVAO == 0)
	{
		float vertices[] =
		{					  //texture ain't texturing
			-1.0f,1.0f,0.0f,  0.0f,1.0f,
			-1.0f,-1.0f,0.0f, 0.0f,0.0f,
			 1.0f,1.0f,0.0f,  1.0f,1.0f,
			 1.0f,-1.0f,0.0f, 1.0f,0.0f,
		};
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
float linear_interpolate(float curr_pos,float target_pos,float deltatime)
{
	return curr_pos + (target_pos - curr_pos) * deltatime;
}
int main()
{
	using namespace std;

	//INITIALISING OPENGL WITH 4.3 FOR COMPUTE SHADER
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	//WINDOW CREATION
	GLFWwindow *window_p = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Compute", NULL, NULL);
	if (window_p == NULL)
	{
		std::cout << "ERROR::WINDOW FAILED" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window_p);
	glfwSetFramebufferSizeCallback(window_p, window_resize_callback);
	//VERTICAL SYNCHRONIZATION TIMEOUT 
	glfwSwapInterval(1);
	
	//LOADING ALL OPENGL POINTERS
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "ERROR::FUNCTION POINTERS WEREN'T LOADED" << std::endl;
		return -1;
	}

	//SHADERSS
	Shader FullQuad("./shaders/FullQuad.vs", "./shaders/FullQuad.ffs");
	ComputeShader compute("./shaders/ComputeShader.ps");

	//LOADING TEXTURE
	unsigned int texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	//cout << "DEBUG1" << std::endl;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	int fCounter = 0;
	float last_pos = 0;

	//debug using query
	GLuint query;
	glGenQueries(1, &query);

	glfwSetTime(0.0);
	while (!glfwWindowShouldClose(window_p))
	{

		
		
		float target_pos = linear_interpolate(last_pos, cos(glfwGetTime()) * 5, deltaTime);
		last_pos = target_pos;
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Smooth continuous motion
		static float animTime = 0.0f;
		
		//compute.setFloat("t",sin(glfwGetTime()*0.2)*5);
		
		compute.use();
		glm::mat4 trans = glm::mat4(1.0f);
		trans = glm::rotate(trans, (float)glfwGetTime(), glm::vec3(0.0, 1.0, 0.0));
		glUniformMatrix4fv(glGetUniformLocation(compute.ID, "roatation"), 1, GL_FALSE, glm::value_ptr(trans));
		// Correct approach in C++ render loop

		std::cout << glfwGetTime() << "\n";
		compute.setFloat("t", tan(glfwGetTime()) * 5);
		compute.setFloat("angle", glfwGetTime());

		glBeginQuery(GL_TIME_ELAPSED,query);
		glDispatchCompute((unsigned int)TEXTURE_WIDTH / 10, (unsigned int)TEXTURE_HEIGHT / 10, 1);
		glEndQuery(GL_TIME_ELAPSED);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		FullQuad.use();
		FullQuad.setInt("tex", 0);
		
		renderQuad();
		//cout << "DEBUG4" << std::endl;
		glfwSwapBuffers(window_p);
		glfwPollEvents();
		
	}
	GLuint64 timeElapsed;
	glGetQueryObjectui64v(query, GL_QUERY_RESULT, &timeElapsed);
	std::cout << timeElapsed;
}
