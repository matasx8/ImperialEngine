#pragma once
#include "Utils/NonCopyable.h"
#include <string>
#include <extern/ASSIMP/Importer.hpp>

namespace imp
{
	class AssetImporter : NonCopyable
	{
	public:
		AssetImporter();
		void Initialize();
		void LoadScene(const std::string& path);

		void Destroy();
	private:

	};
}