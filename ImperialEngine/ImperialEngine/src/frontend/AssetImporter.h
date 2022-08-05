#pragma once
#include "Utils/NonCopyable.h"
#include <string>
#include <filesystem>


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

		void LoadFile(const std::filesystem::path& path);
		void LoadObj(const std::filesystem::path& path);
	};
}