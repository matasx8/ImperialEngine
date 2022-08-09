#pragma once
#include "Utils/NonCopyable.h"
#include <string>
#include <filesystem>

namespace Assimp
{
	class Importer;
}



namespace imp
{
	namespace CmdRsc
	{
		struct MeshCreationRequest;
	}

	class Engine;

	class AssetImporter : NonCopyable
	{
	public:
		AssetImporter(Engine& engine);
		void Initialize();
		void LoadScene(const std::string& path);

		void Destroy();
	private:

		void LoadFile(Assimp::Importer& imp, const std::filesystem::path& path);
		void LoadModel(std::vector<imp::CmdRsc::MeshCreationRequest>& reqs, Assimp::Importer& imp, const std::filesystem::path& path);

		Engine& m_Engine;
	};
}