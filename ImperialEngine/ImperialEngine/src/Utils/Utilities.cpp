#include "Utilities.h"
#include <fstream>

std::vector<std::filesystem::path> OS::GetAllFileNamesInDirectory(const std::string& dir)
{
	std::vector<std::filesystem::path> filePaths;
	if (std::filesystem::exists(dir))
	{
		for (const auto& entry : std::filesystem::directory_iterator(dir))
			filePaths.emplace_back(entry.path());
	}

	return filePaths;
}

const std::shared_ptr<std::string> OS::ReadFileContents(const std::string& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		printf("[Asset Importer] Failed to open a file: %s\n", path.c_str());
		return std::shared_ptr<std::string>();
	}
	size_t fileSize = static_cast<size_t>(file.tellg());
	auto str = std::make_shared<std::string>();
	str->resize(fileSize);

	file.seekg(0);
	file.read(str->data(), fileSize);
	file.close();

	return str;
}
