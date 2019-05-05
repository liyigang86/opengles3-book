#include "esUtil.h"
#include <stdio.h>
typedef struct
{
	// Handle to a program object
	GLuint programObject;

} UserData;

GLuint LoadShader(GLenum type, const char *shaderSrc)
{
	GLuint shader;
	GLint compiled;

	// Create the shader object
	shader = glCreateShader(type);

	if (shader == 0)
	{
		return 0;
	}

	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);

	// Compile the shader
	glCompileShader(shader);

	// Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char *infoLog = malloc(sizeof(char) * infoLen);

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			esLogMessage("Error compiling shader:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;

}

typedef struct
{
	GLuint fbo;
	GLuint renderBufferColor;
}ColorFBO;
ColorFBO colorFBO;

typedef struct
{
	GLuint fbo;
	GLuint renderBufferColor;
	GLuint renderBufferDepthStencil;
}MultiSampleFBO;
MultiSampleFBO multiSampleFBO;

int InitMultiSampleFrameBuffers()
{
	const int MULTISAMPE_NUMBER = 16;

	glGenFramebuffers(1, &multiSampleFBO.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, multiSampleFBO.fbo);

	glGenRenderbuffers(1, &multiSampleFBO.renderBufferColor);
	glBindRenderbuffer(GL_RENDERBUFFER, multiSampleFBO.renderBufferColor);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, MULTISAMPE_NUMBER, GL_RGB8, 320, 240);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, multiSampleFBO.renderBufferColor);

	glGenRenderbuffers(1, &multiSampleFBO.renderBufferDepthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, multiSampleFBO.renderBufferDepthStencil);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, MULTISAMPE_NUMBER, GL_DEPTH24_STENCIL8, 320, 240);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, multiSampleFBO.renderBufferDepthStencil);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, multiSampleFBO.renderBufferDepthStencil);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "framebuffer not complete: %d\n", status);
		return FALSE;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (multiSampleFBO.renderBufferColor != 0) {
		glDeleteRenderbuffers(1, &multiSampleFBO.renderBufferColor);
	}
	if (multiSampleFBO.renderBufferDepthStencil != 0) {
		glDeleteRenderbuffers(1, &multiSampleFBO.renderBufferDepthStencil);
	}
	if (multiSampleFBO.fbo != 0) {
		glDeleteFramebuffers(1, &multiSampleFBO.fbo);
	}

	return TRUE;
}

int Init(ESContext *esContext)
{
	UserData *userData = esContext->userData;
	char vShaderStr[] =
		"#version 300 es                          \n"
		"layout(location = 0) in vec4 vPosition;  \n"
		"void main()                              \n"
		"{                                        \n"
		"   gl_Position = vPosition;              \n"
		"}                                        \n";

	char fShaderStr[] =
		"#version 300 es                              \n"
		"precision mediump float;                     \n"
		"out vec4 fragColor;                          \n"
		"void main()                                  \n"
		"{                                            \n"
		"   fragColor = vec4 ( 1.0, 1.0, 1.0, 1.0 );  \n"
		"}                                            \n";

	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	// Load the vertex/fragment shaders
	vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

	// Create the program object
	programObject = glCreateProgram();

	if (programObject == 0)
	{
		return 0;
	}

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char *infoLog = malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			esLogMessage("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(programObject);
		return FALSE;
	}

	// Store the program object
	userData->programObject = programObject;

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	if (InitMultiSampleFrameBuffers() != TRUE)
	{
		fprintf(stderr, "multisample renderbuffer not supported");
		return FALSE;
	}

	return TRUE;
}

void Draw(ESContext *esContext)
{
	UserData *userData = esContext->userData;
	GLfloat vVertices[] = { 0.0f,  0.5f, 0.0f,
							 -0.5f, -0.5f - 0.7f*1.0f/400.0f, 0.0f,
							 0.5f, -0.5f - 0.7f*1.0f/400.0f, 0.0f
	};

	// Set the viewport
	glViewport(0, 0, esContext->width, esContext->height);

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);

	// Use the program object
	glUseProgram(userData->programObject);

	// Load the vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Shutdown(ESContext *esContext)
{
	UserData *userData = esContext->userData;

	glDeleteProgram(userData->programObject);
}

int esMain(ESContext *esContext)
{
	esContext->userData = malloc(sizeof(UserData));

	esCreateWindow(esContext, "Multisample Framebuffer", 400, 400, ES_WINDOW_RGB | ES_WINDOW_MULTISAMPLE);

	if (!Init(esContext))
	{
		return GL_FALSE;
	}

	esRegisterShutdownFunc(esContext, Shutdown);
	esRegisterDrawFunc(esContext, Draw);

	return GL_TRUE;
}