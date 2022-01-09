#include <iostream>
#include <filesystem>
#include <thread>
#include <future>
#include <chrono>
#include <algorithm>
#include <vector>
#include <string>

struct Result
{
	std::vector<std::string> files;
	std::vector<std::filesystem::path> dirs;

	Result() {}
	Result(Result && r) : dirs(std::move(r.dirs)), files(std::move(r.files)) {}
	Result& operator=(Result && rhs) 
	{
		if (this != &rhs) {
			dirs = std::move(rhs.dirs);
			files = std::move(rhs.files);
			return *this;
		}
	}
};

Result listDirectory(std::filesystem::path&& dir) 
{
	Result result;

	for (auto& p : std::filesystem::directory_iterator(dir))
	{
		//std::cout << p.path().filename().string() << '\n';
		if (p.is_directory())
		{
			result.dirs.push_back(p.path().string());
			auto ftr = std::async(std::launch::async, &listDirectory, p.path().string());

		}
		else
		{
			result.files.push_back(p.path().filename().string());
		}

	}
	return result;

}

int main() {
	auto startTime = std::chrono::system_clock::now();
	std::filesystem::path root("C:\\Users\\myles\\Desktop");

	std::vector<std::filesystem::path> dirsToDo;
	dirsToDo.push_back(root);

	std::vector<std::string> files;

	//listDirectory(std::move(root));
	while (!dirsToDo.empty())
	{
		std::vector<std::future<Result>> futures;
		for (int i = 0; i < 16 && !dirsToDo.empty(); i++)
		{
			auto ftr = std::async(listDirectory, std::move(dirsToDo.back()));
			dirsToDo.pop_back();
			futures.push_back(std::move(ftr));
		} 

		try
		{
			while (!futures.empty())
			{
				std::future ftr = std::move(futures.back());
				futures.pop_back();
				Result res = std::move(ftr.get()); //move constructor is implicitly generated
				std::move(res.files.begin(), res.files.end(), std::back_inserter(files));
				std::move(res.dirs.begin(), res.dirs.end(), std::back_inserter(dirsToDo));

			}
			
			for (std::string s : files)
			{
				std::cout << s << std::endl;
			}
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "Unknown exception\n";
		}
	}
	auto endTime = std::chrono::system_clock::now();
	auto durMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
	std::cout << "\n\nCompleted in " << durMs << std::endl;
}