#include "AssetImporter.h"
#include "Utils/Utilities.h"
#include "backend/EngineCommandResources.h"
#include <extern/ASSIMP/Importer.hpp>
#include <extern/ASSIMP/scene.h>
#include <extern/ASSIMP/postprocess.h>

namespace imp
{
	AssetImporter::AssetImporter()
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

		}

		printf("[Asset Importer] Successfully loaded scene from '%s' with %d files\n", path.c_str(), static_cast<int>(paths.size()));
	}

	void AssetImporter::Destroy()
	{
	}

	void AssetImporter::LoadFile(Assimp::Importer& imp, const std::filesystem::path& path)
	{
		const auto extension = path.extension().string();
		if (extension == "obj")
		{
			std::vector<imp::CmdRsc::MeshCreationRequest> reqs;
			LoadModel(reqs, imp, path);


		}
	}

	static void LoadNode(std::vector<imp::CmdRsc::MeshCreationRequest>& meshList, aiNode* node, const aiScene* scene)
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
			LoadNode(meshList, node->mChildren[i], scene);
	}

	void AssetImporter::LoadModel(std::vector<imp::CmdRsc::MeshCreationRequest>& reqs, Assimp::Importer& imp, const std::filesystem::path& path)
	{
		const aiScene* scene = imp.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
		if (scene == nullptr)
		{
			printf("[Asset Importer] Failed to read '%s' with error: '%s' \n", path.c_str(), imp.GetErrorString());
			return;
		}
		LoadNode(reqs, scene->mRootNode, scene);
		assert(reqs.size());
	}

}