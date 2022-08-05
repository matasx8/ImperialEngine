#include "AssetImporter.h"
#include "Utils/Utilities.h"
#include <extern/ASSIMP/Importer.hpp>

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

	void AssetImporter::LoadFile(const std::filesystem::path& path)
	{
		const auto extension = path.extension().string();
		if (extension == "obj")
			LoadObj(path);
	}

	void AssetImporter::LoadObj(const std::filesystem::path& path)
	{

	}

}