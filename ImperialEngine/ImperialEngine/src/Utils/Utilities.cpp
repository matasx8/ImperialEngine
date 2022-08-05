#include "Utilities.h"

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
