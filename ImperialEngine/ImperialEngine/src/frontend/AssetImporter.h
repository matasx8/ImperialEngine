#pragma once
#include "Utils/NonCopyable.h"
#include <string>
#include <filesystem>
#include <unordered_map>

namespace Assimp
{
	class Importer;
}

namespace tinygltf
{
	class TinyGLTF;
	class Node;
	class Model;
}

namespace Comp
{
	struct GLTFEntity;
	struct GLTFCamera;
}

namespace imp
{
	struct MeshCreationRequest;
	struct MaterialCreationRequest;
	class Engine;

	class AssetImporter : NonCopyable
	{
	public:
		AssetImporter(Engine& engine);
		void LoadScenes(const std::vector<std::string>& paths);
		void LoadMaterials(const std::string& path);
		void LoadComputeProgams(const std::string& path);

		uint32_t GetNumberOfUniqueMeshesLoaded() const;

	private:

		void LoadGLTFScene(const std::filesystem::path& path);
		void LoadGLTFNode(const tinygltf::Node& node, const tinygltf::Model& model, std::vector<MeshCreationRequest>& reqs, std::vector<Comp::GLTFEntity>& entities, std::unordered_map<uint32_t, uint32_t>& meshIdMap, Comp::GLTFCamera& camera);
		void LoadFile(Assimp::Importer& imp, const std::filesystem::path& path);
		void LoadModel(std::vector<imp::MeshCreationRequest>& reqs, Assimp::Importer& imp, const std::filesystem::path& path);
		std::vector<MaterialCreationRequest> LoadShaders(const std::vector<std::filesystem::path>& shaders);
		MaterialCreationRequest LoadShader(const std::string& shader);

		Engine& m_Engine;
		tinygltf::TinyGLTF* m_Loader;
	};
}