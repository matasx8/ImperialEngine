#include "AssetImporter.h"
#include "Utils/Utilities.h"
#include "Utils/GfxUtilities.h"
#include "backend/VariousTypeDefinitions.h"
#include "frontend/Engine.h"
#include "frontend/Components/Components.h"
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#include "extern/STB/stb_image.h"
#include "extern/TINY_GLTF/tiny_gltf.h"
#include "extern/ASSIMP/Importer.hpp"
#include "extern/ASSIMP/scene.h"
#include "extern/ASSIMP/postprocess.h"
#include "GLM/gtc/type_ptr.hpp"
#include "extern/GLM/mat4x4.hpp"
#include "MESHOPTIMIZER/meshoptimizer.h"
#include <unordered_map>

namespace imp
{
	static std::atomic_uint32_t temporaryMeshCounter = 0;

	AssetImporter::AssetImporter(Engine& engine)
		: m_Engine(engine), m_Loader(new tinygltf::TinyGLTF())
	{
	}

	void AssetImporter::LoadScenes(const std::vector<std::string>& paths)
	{
		const auto path = std::filesystem::current_path();
		Assimp::Importer importer;
		if (paths.size())
		{
			for (const auto& file : paths)
			{
				std::filesystem::path path(file);
				// must load data using assimp
				// create entity and from main thread it seems like the object has been loaded and created
				// meanwhile launch render thread command to create vertex and index buffers and upload to gpu
				// entity will have a handle to this data and when time comes to render it'll be magically inplace
				LoadFile(importer, path);
			}
		}
		else
		{
			// By default lets still try load something from Scene/
			const auto files = OS::GetAllFileNamesInDirectory("Scene/");
			for (const auto& path : files)
				LoadFile(importer, path);
		}

		printf("[Asset Importer] Successfully loaded scenes with %d files\n", static_cast<int>(paths.size()));
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

	uint32_t imp::AssetImporter::GetNumberOfUniqueMeshesLoaded() const
	{
		return temporaryMeshCounter;
	}

	void AssetImporter::LoadGLTFScene(const std::filesystem::path& path)
	{
		assert(path.extension().string() == ".gltf" || path.extension().string() == ".glb");
		tinygltf::Model model;
		std::string err;
		std::string warn;

		// TODO gltf: implement failure path
		if(path.extension().string() == ".gltf")
			m_Loader->LoadASCIIFromFile(&model, &err, &warn, path.string());
		else
			m_Loader->LoadBinaryFromFile(&model, &err, &warn, path.string());

		if (err.size()) printf("[Asset Importer] Error: %s\n", err.c_str());
		if (warn.size()) printf("[Asset Importer] Warning: %s\n", warn.c_str());

		assert(model.scenes.size() == 1);
		// large potential for parallel for
		std::vector<MeshCreationRequest> reqs;
		std::unordered_map<uint32_t, uint32_t> meshIdMap;
		std::vector<Comp::GLTFEntity> entities;
		Comp::GLTFCamera camera;
		for (const auto& nodeIdx : model.scenes.front().nodes)
		{
			const auto& node = model.nodes[nodeIdx];
			LoadGLTFNode(node, model, reqs, entities, meshIdMap, camera);
		}

		// means we loaded somekind of camera, try to override exisitng one
		if (camera.valid)
		{
			auto& reg = m_Engine.m_Entities;
			const auto cameras = reg.view<Comp::Transform, Comp::Camera>();
			for (auto ent : cameras)
			{
				auto& transform = cameras.get<Comp::Transform>(ent);
				auto& cam = cameras.get<Comp::Camera>(ent);

				if (cam.preview)
					continue;
				else
				{
					transform = camera.transform;
#if BENCHMARK_MODE
					m_Engine.m_InitialCameraTransform = transform.transform;
#endif
				}
			}
		}

		for (const auto& ent : entities)
		{
			auto& reg = m_Engine.m_Entities;
			const entt::entity mainEntity = reg.create();
			reg.emplace<Comp::Transform>(mainEntity, ent.transform.transform);

			const auto childEntity = reg.create();
			reg.emplace<Comp::Mesh>(childEntity, ent.mesh.meshId);
			reg.emplace<Comp::Material>(childEntity, kDefaultMaterialIndex);
			reg.emplace<Comp::ChildComponent>(childEntity, mainEntity);
		}

		m_Engine.m_Q->add(std::mem_fn(&Engine::Cmd_UploadMeshes), std::make_shared<std::vector<imp::MeshCreationRequest>>(reqs));
	}

	void AssetImporter::LoadGLTFNode(const tinygltf::Node& node, const tinygltf::Model& model, std::vector<MeshCreationRequest>& reqs, std::vector<Comp::GLTFEntity>& entities, std::unordered_map<uint32_t, uint32_t>& meshIdMap, Comp::GLTFCamera& camera)
	{
		auto transform = glm::mat4x4(1.0f);

		if (node.matrix.size())
			transform = glm::make_mat4x4(node.matrix.data());
		else
		{
			if (node.translation.size())
				transform = glm::translate(transform, glm::vec3(glm::make_vec3(node.translation.data())));

			if (node.rotation.size())
				transform *= glm::mat4((glm::quat)glm::make_quat(node.rotation.data()));

			if (node.scale.size()) // TODO: probably VF culling wont work since I don't scale diameter
				transform = glm::scale(transform, glm::vec3(glm::make_vec3(node.scale.data())));
		}

		// special quick hack to import camera orientation
		if (node.name == "Camera")
		{
			camera.transform.transform = transform;
			camera.valid = true;
		}

		for (const auto child : node.children)
			LoadGLTFNode(model.nodes[child], model, reqs, entities, meshIdMap, camera);

		if (meshIdMap.find(node.mesh) != meshIdMap.end())
		{
			MeshCreationRequest req;
			req.id = meshIdMap[node.mesh];
			reqs.push_back(req);

			Comp::GLTFEntity ent;
			ent.transform = { transform };
			ent.mesh = { req.id };
			entities.push_back(ent);

			return;
		}

		if (node.mesh > -1)
		{

			const auto& mesh = model.meshes[node.mesh];

			for (const auto& prim : mesh.primitives)
			{
				MeshCreationRequest req;
				req.id = temporaryMeshCounter.fetch_add(1);

				meshIdMap[node.mesh] = req.id; // can keep rewriting this

				const float* positionBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				size_t vertexCount = 0;

				// TODO nice-to-have: reduce code size by making this a function
				if (prim.attributes.find("POSITION") != prim.attributes.end()) {
					const auto& accessor = model.accessors[prim.attributes.find("POSITION")->second];
					const auto& view = model.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}

				if (prim.attributes.find("NORMAL") != prim.attributes.end()) {
					const auto& accessor = model.accessors[prim.attributes.find("NORMAL")->second];
					const auto& view = model.bufferViews[accessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				if (prim.attributes.find("TEXCOORD_0") != prim.attributes.end()) {
					const auto& accessor = model.accessors[prim.attributes.find("TEXCOORD_0")->second];
					const auto& view = model.bufferViews[accessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				assert(vertexCount);
				for (size_t i = 0; i < vertexCount; i++)
				{
					Vertex vertex;
					std::memcpy(&vertex.vx, &positionBuffer[i * 3], sizeof(float) * 3);

					vertex.nx = meshopt_quantizeHalf(normalsBuffer[i * 3]);
					vertex.ny = meshopt_quantizeHalf(normalsBuffer[i * 3 + 1]);
					vertex.nz = meshopt_quantizeHalf(normalsBuffer[i * 3 + 2]);
					vertex.nw = 0.0f;

					vertex.tu = prim.attributes.find("TEXCOORD_0") != prim.attributes.end() ? meshopt_quantizeHalf(texCoordsBuffer[i * 2]) : 0.0f;
					vertex.tv = prim.attributes.find("TEXCOORD_0") != prim.attributes.end() ? meshopt_quantizeHalf(texCoordsBuffer[i * 2 + 1]) : 0.0f;

					req.vertices.push_back(vertex);
				}

				const auto& accessor = model.accessors[prim.indices];
				const auto& bufferView = model.bufferViews[accessor.bufferView];
				const auto& buffer = model.buffers[bufferView.buffer];

				const auto indexCount = accessor.count;

				const auto FillIndices = [&](const auto& buf)
				{
					for (size_t index = 0; index < accessor.count; index++)
					{
						req.indices.push_back(buf[index]);
					}
				};
				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
				{
					const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					FillIndices(buf);
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: 
				{
					const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					FillIndices(buf);
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
				{
					const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					FillIndices(buf);
					break;
				}
				default:
					printf("[Asset Importer] Error: Index component type %i not supported!\n", accessor.componentType);
					return;
				}

				req.boundingVolume = utils::FindSphereBoundingVolume(req.vertices.data(), req.vertices.size());

				Comp::GLTFEntity ent;
				ent.transform = { transform };
				ent.mesh = { req.id };

				entities.push_back(ent);
				reqs.push_back(req);
			}
		}

		// this is camera index, but we don't care about the actual camera params.
		// only want orientation
		if (node.camera >= 0)
		{
			camera.transform.transform *= transform;
		}
	}

	void AssetImporter::LoadFile(Assimp::Importer& imp, const std::filesystem::path& path)
	{
		static bool tFirstEntityLoaded = true;

		const auto extension = path.extension().string();
		if (extension == ".obj")
		{
			// create main entity, that the renderable entities will point to
			auto& reg = m_Engine.m_Entities;
			const entt::entity mainEntity = reg.create();
			reg.emplace<Comp::Transform>(mainEntity, glm::mat4x4(1.0f));

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
		else if (extension == ".gltf" || extension == ".glb")
		{
			LoadGLTFScene(path);
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
#if CONE_CULLING_ENABLED
		const auto taskShaderPath = shader + ".task.spv";
#endif
		const auto fragmentShaderPath = shader + ".frag.spv";

		MaterialCreationRequest req;
		req.shaderName = shaderPath.stem().string();
		req.vertexSpv = OS::ReadFileContents(vertexShaderPath);
		req.vertexIndSpv = OS::ReadFileContents(vertexIndirectShaderPath);
		req.meshSpv = OS::ReadFileContents(meshShaderPath);
#if CONE_CULLING_ENABLED
		req.taskSpv = OS::ReadFileContents(taskShaderPath);
#endif
		req.fragmentSpv = OS::ReadFileContents(fragmentShaderPath);
		assert(req.vertexSpv.get());
		assert(req.vertexIndSpv.get());
		assert(req.meshSpv.get());
#if CONE_CULLING_ENABLED
		assert(req.taskSpv.get());
#endif
		assert(req.fragmentSpv.get());
		return req;
	}

}