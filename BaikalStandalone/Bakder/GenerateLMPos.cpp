#include "GenerateLMPos.h"
#include <assert.h>
//#include "mesh.h"
//#include "texture.h"

#define CHECK_GL_ERROR assert(glGetError() == 0)

GenerateLMPos::GenerateLMPos()
{
	
}

GenerateLMPos::~GenerateLMPos()
{

}

Baikal::Texture* GenerateLMPos::GenerateLMPosTexture(const Baikal::Mesh *mesh)
{
	//light map size, only for testing
	const int lightmap_w = 1024;
	const int lightmap_h = 1024;

	enum OutDataType
	{
		POSITION,
		TANGENT,
		NORMAL,

		OutDataNum
	};

	// The framebuffer
	GLuint FramebufferName[OutDataNum];
	GLuint renderedTexture[OutDataNum];
	glGenFramebuffers(OutDataNum, FramebufferName); CHECK_GL_ERROR;
	glGenTextures(OutDataNum, renderedTexture); CHECK_GL_ERROR;

	for (int i = 0; i < OutDataNum; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName[i]); CHECK_GL_ERROR;

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, renderedTexture[i]); CHECK_GL_ERROR;

		// Give an empty image to OpenGL ( the last "0" )
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, lightmap_w, lightmap_h, 0, GL_RGB32F, GL_FLOAT, 0); CHECK_GL_ERROR;

		// Poor filtering. Needed !
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_GL_ERROR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_GL_ERROR;

		// Set "renderedTexture" as our colour attachement #0
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, renderedTexture[i], 0); CHECK_GL_ERROR;
	}

	// Set the list of draw buffers.
	GLenum DrawBuffers[OutDataNum];
	for (int i = 0; i < OutDataNum; ++i)
	{
		DrawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}

	//bind shader
	GLuint shader = m_shader_manager->GetProgram("../BaikalStandalone/Kernels/GLSL/generate_lm_pos"); CHECK_GL_ERROR;
	glUseProgram(shader); CHECK_GL_ERROR;


	//bind vertices and indices


	//return NULL;
	glDisable(GL_DEPTH_TEST); CHECK_GL_ERROR;
	glViewport(0, 0, lightmap_w, lightmap_h); CHECK_GL_ERROR;

	glDrawBuffers(OutDataNum, DrawBuffers); CHECK_GL_ERROR;

	return NULL;
}
