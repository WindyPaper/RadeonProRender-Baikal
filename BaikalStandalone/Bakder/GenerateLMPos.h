#ifndef _GENERATE_LM_POS_
#define _GENERATE_LM_POS_

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#define GLFW_INCLUDE_GLCOREARB
#define GLFW_NO_GLU
#include "GLFW/glfw3.h"
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#else
#include <CL/cl.h>
#include <GL/glew.h>
#include <GL/glx.h>
#include "GLFW/glfw3.h"
#endif

#include <map>
#include <memory>
#include "Utils/shader_manager.h"

namespace Baikal
{
	class Mesh;
	class Texture;
}

class GenerateLMPos
{
public:
	GenerateLMPos();
	~GenerateLMPos();

	Baikal::Texture* GenerateLMPosTexture(const Baikal::Mesh *mesh);

private:
	std::map<Baikal::Mesh*, Baikal::Texture*> mesh_texture_map_;
	std::unique_ptr<ShaderManager> m_shader_manager;
};

#endif


