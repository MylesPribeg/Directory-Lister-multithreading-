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
};

class MonitorResult
{
public:
	void put_file(std::string&& file)
	{
		std::lock_guard<std::mutex> lck(mutex); //mutex.lock() and .unlock() aren't called directy in case push_back throws an exception
		result.files.push_back(file);
		//mutex unlocking happens automatically in the destructor
	}
	void put_dir(std::filesystem::path&& dir)
	{
		std::lock_guard<std::mutex> lck(mutex);
		result.dirs.push_back(dir);
	}
	std::vector<std::filesystem::path> get_dirs(int n)
	{
		std::vector<std::filesystem::path> retDirs;
		std::lock_guard<std::mutex> lck(mutex);

		for (int i = 0; i < n && !result.dirs.empty(); i++)
		{
			retDirs.push_back(std::move(result.dirs.back()));
			result.dirs.pop_back();
		}

		return retDirs;
	}
	bool is_dirs_empty()
	{
		std::lock_guard<std::mutex> lck(mutex);
		return result.dirs.empty();
	}
	void print_files()
	{
		std::lock_guard<std::mutex> lck(mutex);
		for (std::string s : result.files)
		{
			std::cout << s << std::endl;
		}
	}
private:
	Result result;
	std::mutex mutex;

};

void listDirectory(std::filesystem::path&& dir, MonitorResult& result) 
{
	for (auto& p : std::filesystem::directory_iterator(dir))
	{
		//std::cout << p.path().filename().string() << '\n';
		if (p.is_directory())
		{
			result.put_dir(p.path().string());
			//auto ftr = std::async(std::launch::async, &listDirectory, p.path().string());

		}
		else
		{
			result.put_file(p.path().filename().string());
		}

	}

}

int main() {
	auto startTime = std::chrono::system_clock::now();
	std::filesystem::path root("C:\\Users\\myles\\Desktop");
	MonitorResult result;
	//std::vector<std::filesystem::path> dirsToDo;
	result.put_dir(std::move(root));

	//listDirectory(std::move(root));
	while (!result.is_dirs_empty())
	{
		std::vector<std::filesystem::path> dirsToDo = result.get_dirs(16);
		std::vector<std::future<void>> futures;
		while(!dirsToDo.empty()) //we know there will only be 16 or fewer items so we don't need to count to 16 in this loop anymore
		{
			auto ftr = std::async(listDirectory, std::move(dirsToDo.back()), std::ref(result));
			dirsToDo.pop_back();
			futures.push_back(std::move(ftr));
		} 

		try
		{
			while (!futures.empty())
			{
				std::future ftr = std::move(futures.back());
				futures.pop_back();
				ftr.wait(); //could also use get(). Would be the same as get() wouldn't return anything in this case
			}
			
			result.print_files();
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