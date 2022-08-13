#include "AssetImporter.h"
#include "Utils/Utilities.h"
#include "backend/EngineCommandResources.h"
#include "frontend/Engine.h"
#include "frontend/Components/Components.h"
#include <extern/ASSIMP/Importer.hpp>
#include <extern/ASSIMP/scene.h>
#include <extern/ASSIMP/postprocess.h>
#include <extern/GLM/mat4x4.hpp>

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
		const auto paths = OS::GetAllFileNamesInDirectory("Scene\\");
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

	void AssetImporter::Destroy()
	{
	}

	void AssetImporter::LoadFile(Assimp::Importer& imp, const std::filesystem::path& path)
	{
		const auto extension = path.extension().string();
		if (extension == ".obj")
		{
			// create main entity, that the renderable entities will point to
			const entt::entity mainEntity = m_Engine.m_Entities.create();
			auto& reg = m_Engine.m_Entities;
			reg.emplace<Comp::Transform>(mainEntity, glm::mat4x4(1.0f));

			std::vector<imp::CmdRsc::MeshCreationRequest> reqs;
			LoadModel(reqs, imp, path);
			for (auto& req : reqs)
			{
				// renderable entity is basically a mesh and material
				// mesh component will point to IndexedVerts entity that'll have an index and vertex buffer
				const auto renderable = reg.create();
				// create the indexed verts entity so backend knows where to assign indices and vertices
				// frontend doesn't have to know about actual VkBuffers..
				const auto vertexData = reg.create(); // not sure if this would be thread safe.. answer: it's not!
				reg.emplace<Comp::IndexedVertexBuffers>(vertexData);	// also assign the vertex component, backend can't add anything to registry
				req.vertexData = vertexData;
				reg.emplace<Comp::Mesh>(renderable);
			}
			m_Engine.m_SyncPoint->arrive_and_wait();
			//m_Engine.SyncRenderThread();	// and then insert another barrier for first frame
			// TODO: put this somewhere higher in the callstack so we upload all the meshes at the same time
			m_Engine.m_Q->add(std::mem_fn(&Engine::Cmd_UploadMeshes), std::make_shared<std::vector<imp::CmdRsc::MeshCreationRequest>>(reqs));

		}
	}

	static void LoadMesh(std::vector<imp::CmdRsc::MeshCreationRequest>& meshList, aiNode* node, const aiScene* scene)
	{
		// go through each mesh at this node and create it, then add it to our meshlist
		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			imp::CmdRsc::MeshCreationRequest req;
			const auto& mesh = scene->mMeshes[node->mMeshes[i]];

			//resize vertex list to hold all vertices for mesh
			req.vertices.resize(mesh->mNumVertices);

			for (size_t j = 0; j < mesh->mNumVertices; j++)
			{
				// set position
				req.vertices[j].pos = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };

				// set tex coords (if they exist)
				if (mesh->mTextureCoords[0])
				{
					req.vertices[j].tex = { mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y };
				}
				else
				{
					req.vertices[j].tex = { 0.0f, 0.0f };
				}

				req.vertices[j].norm = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };
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

			meshList.push_back(req);
		}

		// go through each node attached to this node and load it, then append their meshes to this node's mesh list
		for (size_t i = 0; i < node->mNumChildren; i++)
			LoadMesh(meshList, node->mChildren[i], scene);
	}

	void AssetImporter::LoadModel(std::vector<imp::CmdRsc::MeshCreationRequest>& reqs, Assimp::Importer& imp, const std::filesystem::path& path)
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

}