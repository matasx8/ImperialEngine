#include "AssetImporter.h"
#include "Utils/Utilities.h"
#include "Utils/GfxUtilities.h"
#include "backend/VariousTypeDefinitions.h"
#include "frontend/Engine.h"
#include "frontend/Components/Components.h"
#include <extern/ASSIMP/Importer.hpp>
#include <extern/ASSIMP/scene.h>
#include <extern/ASSIMP/postprocess.h>
#include <extern/GLM/mat4x4.hpp>
#include <MESHOPTIMIZER/meshoptimizer.h>

namespace imp
{
	AssetImporter::AssetImporter(Engine& engine)
		: m_Engine(engine)
	{
	}

	void AssetImporter::Initialize()
	{
	}

	void AssetImporter::LoadScene(const std::string& path)
	{
		const auto paths = OS::GetAllFileNamesInDirectory(path);
		Assimp::Importer importer;
		for (const auto& file : paths)
		{
			// must load data using assimp
			// create entity and from main thread it seems like the object has been loaded and created
			// meanwhile launch render thread command to create vertex and index buffers and upload to gpu
			// entity will have a handle to this data and when time comes to render it'll be magically inplace
			LoadFile(importer, file);
		}

		printf("[Asset Importer] Successfully loaded scene from '%s' with %d files\n", path.c_str(), static_cast<int>(paths.size()));
	}

	void AssetImporter::LoadMaterials(const std::string& path)
	{
		// it probably makes sense to use glslang reflector, but for speed hardcode first material
		const auto paths = OS::GetAllFileNamesInDirectory(path);

		// load files and compose material creation requests
		const auto materialCreationReqs = LoadShaders(paths);

		// parse material creation requests and create materials
		m_Engine.m_Q->add(std::mem_fn(&Engine::Cmd_UploadMaterials), std::make_shared<std::vector<imp::MaterialCreationRequest>>(materialCreationReqs));

		// should also create material entities.
		// since we only have one no need for now.
		// Note for the future: No way to map material to mesh right now and probably no need. Since it's currently more of an editor
		// where we load the pure assets and THEN in the editor we can assign a material to an entity. After serializing gets implemented
		// we should be able to load the serialized scene and know which entity has which component and etc.

		printf("[Asset Importer] Successfully loaded materials from '%s' with %i materials\n", path.c_str(), static_cast<int>(paths.size()));
	}

	void AssetImporter::LoadComputeProgams(const std::string& path)
	{
		const auto paths = OS::GetAllFileNamesInDirectory(path);

		std::vector<ComputeProgramCreationRequest> reqs;
		for (const auto& shaderPath : paths)
		{
			if (shaderPath.string().find(".comp") == std::string::npos)
				continue;

			reqs.emplace_back(shaderPath.filename().stem().stem().stem().string(), OS::ReadFileContents(shaderPath.string()));
		}
		m_Engine.m_Q->add(std::mem_fn(&Engine::Cmd_UploadComputePrograms), std::make_shared<std::vector<imp::ComputeProgramCreationRequest>>(reqs));
		printf("[Asset Importer] Successfully loaded compute programs from '%s' with %i total shaders\n", path.c_str(), static_cast<int>(paths.size()));
	}

	void AssetImporter::LoadFile(Assimp::Importer& imp, const std::filesystem::path& path)
	{
		// TODO: currently we load every obj file in Scenes folder and also create an entity for it
		// I want to load the meshes but not create an entity for each one. There's a button
		// in the UI to do that.
		static bool tFirstEntityLoaded = false;

		const auto extension = path.extension().string();
		if (extension == ".obj")
		{
			// create main entity, that the renderable entities will point to
			auto& reg = m_Engine.m_Entities;
			const entt::entity mainEntity = reg.create();
			reg.emplace<Comp::Transform>(mainEntity, glm::mat4x4(1.0f));

			static uint32_t temporaryMeshCounter = 0;
			std::vector<imp::MeshCreationRequest> reqs;
			LoadModel(reqs, imp, path);

			for (auto& req : reqs)
			{
				if (!tFirstEntityLoaded)
				{
					const auto childEntity = reg.create();
					reg.emplace<Comp::Mesh>(childEntity, temporaryMeshCounter); // temporary mesh counter will be used to point to mesh id and bounding volume id 
																				//(because 1:1 ratio for mesh and BV)
					reg.emplace<Comp::Material>(childEntity, kDefaultMaterialIndex);
					reg.emplace<Comp::ChildComponent>(childEntity, mainEntity);
				}

				// TODO acceleration-part-1: create BV once
				const auto BV = utils::FindSphereBoundingVolume(req.vertices.data(), req.vertices.size());
				m_Engine.m_Gfx.m_BVs[temporaryMeshCounter] = BV;

				// this counter can be used to identify the mesh or BV
				req.id = static_cast<uint32_t>(temporaryMeshCounter);
				temporaryMeshCounter++;
			}
			tFirstEntityLoaded = true;

			// TODO: this is needed now because on ST mode the UploadMeshes command would get executed instantly
			// that doesn't fit now because we actually should store the creation requests somewhere and then add the command when the game loop starts
			// TODO: this is probably not needed anymore ?
			if(m_Engine.m_EngineSettings.threadingMode == kEngineSingleThreaded)
				m_Engine.m_SyncPoint->arrive_and_wait();

			// TODO: put this somewhere higher in the callstack so we upload all the meshes at the same time
			m_Engine.m_Q->add(std::mem_fn(&Engine::Cmd_UploadMeshes), std::make_shared<std::vector<imp::MeshCreationRequest>>(reqs));
		}
	}

	static void LoadMesh(std::vector<imp::MeshCreationRequest>& meshList, aiNode* node, const aiScene* scene)
	{
		// go through each mesh at this node and create it, then add it to our meshlist
		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			imp::MeshCreationRequest req;
			const auto& mesh = scene->mMeshes[node->mMeshes[i]];

			//resize vertex list to hold all vertices for mesh
			req.vertices.resize(mesh->mNumVertices);

			static_assert(sizeof(Vertex) == 24);

			for (size_t j = 0; j < mesh->mNumVertices; j++)
			{
				// set position
				req.vertices[j].vx = mesh->mVertices[j].x;
				req.vertices[j].vy = mesh->mVertices[j].y;
				req.vertices[j].vz = mesh->mVertices[j].z;

				// set tex coords (if they exist)
				if (mesh->mTextureCoords[0])
				{
					req.vertices[j].tu = meshopt_quantizeHalf(mesh->mTextureCoords[0][j].x);
					req.vertices[j].tv = meshopt_quantizeHalf(mesh->mTextureCoords[0][j].y);
				}
				else
				{
					req.vertices[j].tu = 0;
				 	req.vertices[j].tv = 0;
				}	

				req.vertices[j].nx = meshopt_quantizeHalf(mesh->mNormals[j].x);
				req.vertices[j].ny = meshopt_quantizeHalf(mesh->mNormals[j].y);
				req.vertices[j].nz = meshopt_quantizeHalf(mesh->mNormals[j].z);
				req.vertices[j].nw = 0;
			}

			// iterate over indices through faces and copy across
			for (size_t j = 0; j < mesh->mNumFaces; j++)
			{
				// get a face
				aiFace face = mesh->mFaces[j];

				// go through face's indices and add to list
				for (size_t k = 0; k < face.mNumIndices; k++)
				{
					req.indices.push_back(face.mIndices[k]);
				}
			}

			const auto BV = utils::FindSphereBoundingVolume(req.vertices.data(), req.vertices.size());
			req.boundingVolume = BV;

			meshList.push_back(req);
		}

		// go through each node attached to this node and load it, then append their meshes to this node's mesh list
		for (size_t i = 0; i < node->mNumChildren; i++)
			LoadMesh(meshList, node->mChildren[i], scene);
	}

	void AssetImporter::LoadModel(std::vector<imp::MeshCreationRequest>& reqs, Assimp::Importer& imp, const std::filesystem::path& path)
	{
		const aiScene* scene = imp.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
		if (scene == nullptr)
		{
			printf("[Asset Importer] Failed to read '%s' with error: '%s' \n", path.c_str(), imp.GetErrorString());
			return;
		}
		LoadMesh(reqs, scene->mRootNode, scene);
		assert(reqs.size());
	}

	std::vector<MaterialCreationRequest> AssetImporter::LoadShaders(const std::vector<std::filesystem::path>& shaders)
	{
		std::unordered_set<std::string> shaderSet;

		for (const auto& shader : shaders)
		{
			// skip compute
			if (shader.string().find(".comp") != std::string::npos)
				break;

			shaderSet.insert(shader.parent_path().string() + "/" + shader.stem().stem().stem().string()); //TODO: fix this nonsense
		}

		std::vector<MaterialCreationRequest> reqs;
		for (const auto& shader : shaderSet)
			reqs.emplace_back(LoadShader(shader));

		return reqs;
	}

	MaterialCreationRequest AssetImporter::LoadShader(const std::string& shader)
	{
		// Easy way to get just the shader name
		std::filesystem::path shaderPath(shader);
		const auto vertexShaderPath = shader + ".vert.spv";
		const auto vertexIndirectShaderPath = shader + ".ind.vert.spv";
		const auto meshShaderPath = shader + ".mesh.spv";
		const auto fragmentShaderPath = shader + ".frag.spv";

		MaterialCreationRequest req;
		req.shaderName = shaderPath.stem().string();
		req.vertexSpv = OS::ReadFileContents(vertexShaderPath);
		req.vertexIndSpv = OS::ReadFileContents(vertexIndirectShaderPath);
		req.meshSpv = OS::ReadFileContents(meshShaderPath);
		req.fragmentSpv = OS::ReadFileContents(fragmentShaderPath);
		assert(req.vertexSpv.get());
		assert(req.vertexIndSpv.get());
		assert(req.meshSpv.get());
		assert(req.fragmentSpv.get());
		return req;
	}

}