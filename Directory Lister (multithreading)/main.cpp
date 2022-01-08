#include <iostream>
#include <filesystem>
#include <thread>
#include <future>
#include <chrono>
#include <algorithm>
#include <vector>
#include <string>

std::vector<std::string> listDirectory(std::filesystem::path&& dir) 
{
	std::vector<std::string> listing;
	std::string dirStr("\n> ");
	dirStr += dir.string() + ":\n ";

	listing.push_back(dirStr);

	//std::cout << "root name " << std::filesystem::exists(dir);

	std::vector<std::future<std::vector<std::string>>> futures;
	for (auto& p : std::filesystem::directory_iterator(dir))
	{
		std::cout << p.path().filename().string() << '\n';
		if (p.is_directory())
		{
			auto ftr = std::async(std::launch::async, &listDirectory, p.path().string());
			futures.push_back(std::move(ftr));
		}
		else
		{
			listing.push_back(std::move(p.path().filename().string()));
		}


	}

	std::for_each(futures.begin(), futures.end(), [&](std::future<std::vector<std::string>>& ftr) {
		std::vector<std::string> list = ftr.get();
		std::copy(list.begin(), list.end(), std::back_inserter(listing));

		});


	return listing;

}

int main() {

	std::filesystem::path root("C:\\Users\\myles\\Desktop");

	listDirectory(std::move(root));
	auto ftr = std::async(std::launch::async, &listDirectory, root);

	std::vector<std::string> listing = ftr.get();
	for (std::string s : listing)
	{
		std::cout << s << std::endl;
	}
	
}