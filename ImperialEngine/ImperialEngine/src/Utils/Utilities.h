#pragma once
#include <vector>
#include <string>
#include <filesystem>

namespace OS
{
	std::vector<std::filesystem::path> GetAllFileNamesInDirectory(const std::string& dir);
	const std::shared_ptr<std::string> ReadFileContents(const std::string& path);
}