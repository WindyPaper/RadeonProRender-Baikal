/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

#include "scene_io.h"
#include "image_io.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/material.h"
#include "SceneGraph/light.h"
#include "SceneGraph/texture.h"
#include "math/mathutils.h"

#include "SceneGraph/uberv2material.h"
#include "SceneGraph/inputmaps.h"

#include <string>
#include <map>
#include <set>
#include <cassert>

//#include "tiny_obj_loader.h"
#include "Utils/log.h"
#include "ofbx.h"

#include "Importer.hpp"
#include "scene.h"
#include "postprocess.h"

namespace Baikal
{
	// Obj scene loader
	class FBX_Loader : public SceneIo::Loader
	{
	public:
		// Load scene from file
		Scene1::Ptr LoadScene(std::string const& filename, std::string const& basepath) const override;
		FBX_Loader() : SceneIo::Loader("fbx", this)
		{			
			SceneIo::RegisterLoader("fbx", this);
		}
		~FBX_Loader()
		{
			SceneIo::UnregisterLoader("fbx");
		}

	private:
		Material::Ptr TranslateMaterialUberV2(ImageIo const& image_io, aiMaterial const& mat, std::string const& basepath, Scene1& scene) const;

		mutable std::map<std::string, Material::Ptr> m_material_cache;
	};

	// Create static object to register loader. This object will be used as loader
	static FBX_Loader obj_loader;


	Scene1::Ptr FBX_Loader::LoadScene(std::string const& filename, std::string const& basepath) const
	{
		//using namespace tinyobj;

		auto image_io(ImageIo::CreateImageIo());

		// Loader data
		//std::vector<shape_t> objshapes;
		//std::vector<material_t> objmaterials;
		//attrib_t attrib;

		// Allocate scene
		auto scene = Scene1::Create();

		// Try loading file
		LogInfo("Loading a scene from fbx: ", filename, " ... ");
		std::string err;

		FILE* fp = fopen(filename.c_str(), "rb");
		
		Assimp::Importer importer;
		unsigned int flags = aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_MakeLeftHanded |
			aiProcess_PreTransformVertices |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_OptimizeMeshes |
			aiProcess_FlipUVs |
			aiProcess_FlipWindingOrder;
		const aiScene* import_fbx_scene = importer.ReadFile(filename, flags);
		if (import_fbx_scene == NULL)
		{
			std::string error_code = importer.GetErrorString();
			return NULL;
		}
		
		unsigned int mesh_num = import_fbx_scene->mNumMeshes;
		
		int mat_num = import_fbx_scene->mNumMaterials;

		std::set<Material::Ptr> emissives;
		std::vector<Material::Ptr> materials(mat_num);

		for (int mat_i = 0; mat_i < mat_num; ++mat_i)
		{
			materials[mat_i] = TranslateMaterialUberV2(*image_io, *import_fbx_scene->mMaterials[mat_i], basepath, *scene);


			// Add to emissive subset if needed
			if (materials[mat_i]->HasEmission())
			{
				emissives.insert(materials[mat_i]);
			}
		}

		//auto res = LoadObj(&attrib, &objshapes, &objmaterials, &err, filename.c_str(), basepath.c_str(), true);
		//if (!res)
		//{
		//	throw std::runtime_error(err);
		//}
		//LogInfo("Success\n");

		//// Allocate scene
		//auto scene = Scene1::Create();

		// Enumerate and translate materials
		// Keep track of emissive subset

		for (unsigned int mesh_i = 0; mesh_i < mesh_num; ++mesh_i)
		{
			aiMesh *mesh_ptr = import_fbx_scene->mMeshes[mesh_i];
			
						
			//const ofbx::Geometry *geo_ptr = mesh_ptr->getGeometry();

			int vertice_num = mesh_ptr->mNumVertices;
			std::vector<RadeonRays::float3> vertices_vec(vertice_num);
			std::vector<RadeonRays::float3> normals_vec(vertice_num);
			std::vector<RadeonRays::float2> uv_vec(vertice_num);
			std::vector<unsigned int> indices;

			for (int i = 0; i < vertice_num; ++i)
			{
				vertices_vec[i] = RadeonRays::float3(mesh_ptr->mVertices[i].x, mesh_ptr->mVertices[i].y, mesh_ptr->mVertices[i].z);
				normals_vec[i] = RadeonRays::float3(mesh_ptr->mNormals[i].x, mesh_ptr->mNormals[i].y, mesh_ptr->mNormals[i].z);
				uv_vec[i] = RadeonRays::float2(mesh_ptr->mTextureCoords[0][i].x, mesh_ptr->mTextureCoords[0][i].y);				
			}

			// Copy the index data
			int num_indices = mesh_ptr->mNumFaces * 3;
			int index_size = 2;
			indices.resize(num_indices, 0);
			const unsigned int numTriangles = mesh_ptr->mNumFaces;
			for (unsigned int triIdx = 0; triIdx < numTriangles; ++triIdx)
			{
				void* triStart = &indices[triIdx * 3];
				const aiFace& tri = mesh_ptr->mFaces[triIdx];
				//if (index_size == 4)
				memcpy(triStart, tri.mIndices, sizeof(uint32_t) * 3);
				/*else
				{
					uint16_t* triIndices = reinterpret_cast<uint16_t*>(triStart);
					for (uint32_t i = 0; i < 3; ++i)
						triIndices[i] = uint16_t(tri.mIndices[i]);
				}*/
			}

			auto mesh = Mesh::Create();
			mesh->SetVertices(&vertices_vec[0], vertice_num);
			mesh->SetNormals(&normals_vec[0], vertice_num);
			mesh->SetUVs(&uv_vec[0], vertice_num);
			mesh->SetIndices(reinterpret_cast<std::uint32_t const*>(&indices[0]), num_indices);
			
			mesh->SetMaterial(materials[mesh_ptr->mMaterialIndex]);

			scene->AttachShape(mesh);
		}

		


		//for (int i = 0; i < (int)objmaterials.size(); ++i)
		//{
		//	// Translate material
		//	materials[i] = TranslateMaterialUberV2(*image_io, objmaterials[i], basepath, *scene);

		//	// Add to emissive subset if needed
		//	if (materials[i]->HasEmission())
		//	{
		//		emissives.insert(materials[i]);
		//	}
		//}

		//// Enumerate all shapes in the scene
		//for (int s = 0; s < (int)objshapes.size(); ++s)
		//{
		//	const auto& shape = objshapes[s];

		//	// Find all materials used by this shape.
		//	std::set<int> used_materials(std::begin(shape.mesh.material_ids), std::end(shape.mesh.material_ids));

		//	// Split the mesh into multiple meshes, each with only one material.
		//	for (int used_material : used_materials)
		//	{
		//		// Map from old index to new index.
		//		auto index_comp = [](index_t const& a, index_t const& b)
		//		{
		//			return (a.vertex_index < b.vertex_index)
		//				|| (a.vertex_index == b.vertex_index && a.normal_index < b.normal_index)
		//				|| (a.vertex_index == b.vertex_index && a.normal_index == b.normal_index
		//					&& a.texcoord_index < b.texcoord_index);
		//		};
		//		std::map<index_t, unsigned int, decltype(index_comp)> used_indices(index_comp);

		//		// Remapped indices.
		//		std::vector<unsigned int> indices;

		//		// Collected vertex/normal/texcoord data.
		//		std::vector<float> vertices, normals, texcoords;

		//		// Go through each face in the mesh.
		//		for (size_t i = 0; i < shape.mesh.material_ids.size(); ++i)
		//		{
		//			// Skip faces which don't use the current material.
		//			if (shape.mesh.material_ids[i] != used_material) continue;

		//			const int num_face_vertices = shape.mesh.num_face_vertices[i];
		//			assert(num_face_vertices == 3 && "expected triangles");
		//			// For each vertex index of this face.
		//			for (int j = 0; j < num_face_vertices; ++j)
		//			{
		//				index_t old_index = shape.mesh.indices[num_face_vertices * i + j];

		//				// Collect vertex/normal/texcoord data. Avoid inserting the same data twice.
		//				auto result = used_indices.emplace(old_index, (unsigned int)(vertices.size() / 3));
		//				if (result.second) // Did insert?
		//				{
		//					// Push the new data.
		//					for (int k = 0; k < 3; ++k)
		//					{
		//						vertices.push_back(attrib.vertices[3 * old_index.vertex_index + k]);
		//					}

		//					for (int k = 0; k < 3; ++k)
		//					{
		//						normals.push_back(attrib.normals[3 * old_index.normal_index + k]);
		//					}

		//					for (int k = 0; k < 2; ++k)
		//					{
		//						// If an uv is present
		//						if (old_index.texcoord_index != -1)
		//						{
		//							texcoords.push_back(attrib.texcoords[2 * old_index.texcoord_index + k]);
		//						}
		//						else
		//						{
		//							texcoords.push_back(0.0f);
		//						}
		//					}

		//				}

		//				const unsigned int new_index = result.first->second;
		//				indices.push_back(new_index);
		//			}
		//		}

		//		// Create empty mesh
		//		auto mesh = Mesh::Create();

		//		// Set vertex and index data
		//		auto num_vertices = vertices.size() / 3;
		//		mesh->SetVertices(&vertices[0], num_vertices);

		//		auto num_normals = normals.size() / 3;
		//		mesh->SetNormals(&normals[0], num_normals);

		//		auto num_uvs = texcoords.size() / 2;
		//		mesh->SetUVs(&texcoords[0], num_uvs);

		//		// Set indices
		//		auto num_indices = indices.size();
		//		mesh->SetIndices(reinterpret_cast<std::uint32_t const*>(&indices[0]), num_indices);

		//		// Set material

		//		if (used_material >= 0)
		//		{
		//			mesh->SetMaterial(materials[used_material]);
		//		}

		//		// Attach to the scene
		//		scene->AttachShape(mesh);

		//		// If the mesh has emissive material we need to add area light for it
		//		if (used_material >= 0 && emissives.find(materials[used_material]) != emissives.cend())
		//		{
		//			// Add area light for each polygon of emissive mesh
		//			for (std::size_t l = 0; l < mesh->GetNumIndices() / 3; ++l)
		//			{
		//				auto light = AreaLight::Create(mesh, l);
		//				scene->AttachLight(light);
		//			}
		//		}
		//	}
		//}

		// TODO: temporary code to add directional light
		auto light = DirectionalLight::Create();
		light->SetDirection(RadeonRays::float3(.1f, -1.f, -.1f));
		light->SetEmittedRadiance(RadeonRays::float3(1.f, 1.f, 1.f));

		scene->AttachLight(light);

		return scene;
	}
	Material::Ptr  FBX_Loader::TranslateMaterialUberV2(ImageIo const& image_io, aiMaterial const& mat, std::string const& basepath, Scene1& scene) const
	{
		aiString mat_name;
		mat.Get(AI_MATKEY_NAME, mat_name);

		auto iter = m_material_cache.find(mat_name.C_Str());

		if (iter != m_material_cache.cend())
		{
			return iter->second;
		}

		UberV2Material::Ptr material = UberV2Material::Create();

		aiColor3D color;
		RadeonRays::float3 emission;
		if (AI_SUCCESS == mat.Get(AI_MATKEY_COLOR_EMISSIVE, color))
		{
			emission = RadeonRays::float3(color.r, color.g, color.b);
		}		

		bool apply_gamma = true;

		uint32_t material_layers = 0;
		auto uberv2_set_texture = [](UberV2Material::Ptr material, const std::string input_name, Texture::Ptr texture, bool apply_gamma)
		{
			if (apply_gamma)
			{
				material->SetInputValue(input_name.c_str(),
					InputMap_Pow::Create(
						InputMap_Sampler::Create(texture),
						InputMap_ConstantFloat::Create(2.2f)));
			}
			else
			{
				material->SetInputValue(input_name.c_str(), InputMap_Sampler::Create(texture));
			}
		};
		auto uberv2_set_bump_texture = [](UberV2Material::Ptr material, Texture::Ptr texture)
		{
			auto bump_sampler = InputMap_SamplerBumpMap::Create(texture);
			auto bump_remap = Baikal::InputMap_Remap::Create(
				Baikal::InputMap_ConstantFloat3::Create(RadeonRays::float3(0.f, 1.f, 0.f)),
				Baikal::InputMap_ConstantFloat3::Create(RadeonRays::float3(-1.f, 1.f, 0.f)),
				bump_sampler);
			material->SetInputValue("uberv2.shading_normal", bump_remap);

		};

		//const ofbx::Texture *diffuse_tex_ptr = mat.getTexture(ofbx::Texture::TextureType::DIFFUSE);
		
		//// Check emission layer
		aiString ai_emissive_str;
		if (emission.sqnorm() > 0)
		{
			material_layers |= UberV2Material::Layers::kEmissionLayer;			
			if (mat.GetTexture(aiTextureType_EMISSIVE, 0, &ai_emissive_str) == aiReturn_SUCCESS)
			{				
				//ai_emissive_str->getFileName().toString(diff_tex_path);
				auto texture = LoadTexture(image_io, scene, basepath, ai_emissive_str.C_Str());
				uberv2_set_texture(material, "uberv2.emission.color", texture, apply_gamma);
			}
			else
			{
				material->SetInputValue("uberv2.emission.color", InputMap_ConstantFloat3::Create(emission));
			}
		}

		//ofbx::Color diffuse_color = mat.getDiffuseColor();
		/*ofbx::Color diffuse_color;
		diffuse_color.r = 1.0f;
		diffuse_color.g = 1.0f;
		diffuse_color.b = 1.0f;*/

		aiColor3D specular_color_value;
		mat.Get(AI_MATKEY_COLOR_SPECULAR, specular_color_value);
		auto s = RadeonRays::float3(specular_color_value.r, specular_color_value.g, specular_color_value.b);

		aiColor3D transmit_value;
		mat.Get(AI_MATKEY_COLOR_TRANSPARENT, transmit_value);
		auto r = RadeonRays::float3(transmit_value.r, transmit_value.g, transmit_value.b);
		//auto d = RadeonRays::float3(diffuse_color.r, diffuse_color.g, diffuse_color.b);

		float specular_exp = 0.0f;
		mat.Get(AI_MATKEY_SHININESS, specular_exp);

		float reflective_val = 1.0f;
		//mat.Get(AI_MATKEY_REFLECTIVITY, reflective_val);

		aiColor3D reflective_color;
		mat.Get(AI_MATKEY_COLOR_REFLECTIVE, reflective_color);

		float refractive_val = 0.0f;
		mat.Get(AI_MATKEY_REFRACTI, refractive_val);		

		auto default_ior = Baikal::InputMap_ConstantFloat::Create(1.5f);
		auto default_roughness = Baikal::InputMap_ConstantFloat::Create(0.01f);
		auto default_one = Baikal::InputMap_ConstantFloat3::Create(RadeonRays::float3(1.0, 0.0, 0.0));

		// Check refraction layer
		if (refractive_val > FLOAT_EPS)
		{
			material_layers |= UberV2Material::Layers::kRefractionLayer;
			material->SetInputValue("uberv2.refraction.ior", default_ior);
			material->SetInputValue("uberv2.refraction.roughness", default_roughness);
			material->SetInputValue("uberv2.refraction.color", InputMap_ConstantFloat3::Create(r));
		}

		// Check specular layer
		if (reflective_val > FLOAT_EPS)
		{
			material_layers |= UberV2Material::Layers::kReflectionLayer;
			material->SetInputValue("uberv2.reflection.ior", default_ior);
			auto roughness = Baikal::InputMap_ConstantFloat::Create(0.000001f);
			material->SetInputValue("uberv2.reflection.roughness", roughness);
			material->SetInputValue("uberv2.reflection.metalness", default_one);

			aiString specular_tex;
			if (mat.GetTexture(aiTextureType_SPECULAR, 0, &specular_tex) == aiReturn_SUCCESS)
			{
				auto texture = LoadTexture(image_io, scene, basepath, specular_tex.C_Str());
				uberv2_set_texture(material, "uberv2.reflection.color", texture, apply_gamma);
			}
			else
			{
				//auto refle_c = RadeonRays::float3(reflective_color.r, reflective_color.g, reflective_color.b);
				material->SetInputValue("uberv2.reflection.color", InputMap_ConstantFloat3::Create(1));
			}
		}

		// Check if we have bump map
		aiString ai_normal_str;
		if (mat.GetTexture(aiTextureType_NORMALS, 0, &ai_normal_str) == aiReturn_SUCCESS)
		{
			material_layers |= UberV2Material::Layers::kShadingNormalLayer;			
			auto texture = LoadTexture(image_io, scene, basepath, ai_normal_str.C_Str());
			uberv2_set_bump_texture(material, texture);
		}

		// Finally add diffuse layer
		material_layers |= UberV2Material::Layers::kDiffuseLayer;
		aiString ai_diffuse_str;
		if (mat.GetTexture(aiTextureType_DIFFUSE, 0, &ai_diffuse_str) == aiReturn_SUCCESS)
		{			
			auto texture = LoadTexture(image_io, scene, basepath, ai_diffuse_str.C_Str());
			uberv2_set_texture(material, "uberv2.diffuse.color", texture, apply_gamma);
		}
		else
		{
			aiColor3D diffuse_color(0.f, 0.f, 1.f);
			mat.Get(AI_MATKEY_COLOR_DIFFUSE,  diffuse_color);
			auto diff_baikal = RadeonRays::float3(diffuse_color.r, diffuse_color.g, diffuse_color.b);
			material->SetInputValue("uberv2.diffuse.color", InputMap_ConstantFloat3::Create(diff_baikal));
		}

		// Set material name
		material->SetName(mat_name.C_Str());
		material->SetLayers(material_layers);

		m_material_cache.emplace(std::make_pair(mat_name.C_Str(), material));

		return material;
	}
}
