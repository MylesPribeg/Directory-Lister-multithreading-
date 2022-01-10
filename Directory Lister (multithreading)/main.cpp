#include <iostream>
#include <filesystem>
#include <thread>
#include <future>
#include <chrono>
#include <algorithm>
#include <vector>
#include <string>
#include <condition_variable>
#include <deque>

const int NUM_THREADS = 10;

template<class T>
class MsgQueue
{
	std::deque<T> queue;
	std::condition_variable cond;
	std::mutex mutex;
public:
	void send(T&& msg)
	{
		{
			std::lock_guard<std::mutex> lck(mutex);
			queue.push_front(std::move(msg));
		}
		cond.notify_one();
	}
	T recieve()
	{
		std::unique_lock<std::mutex> lck(mutex);
		cond.wait(lck, [&] {return !queue.empty(); }); // this is equivalent to putting it in a while not empty loop
		T msg = std::move(queue.back());
		queue.pop_back();
		return msg;
	}
};

void listDirectoryServer(MsgQueue<std::filesystem::path>& dirQueue, MsgQueue<std::string>& fileQueue)
{
	while (1) 
	{
		std::filesystem::path dir = dirQueue.recieve();
		for (auto& p : std::filesystem::directory_iterator(dir))
		{
			if (p.is_directory())
			{
				dirQueue.send(std::filesystem::path(p.path()));
			}
			else
			{
				fileQueue.send(p.path().filename().string());
			}
		}
	}
}

void printServer(MsgQueue<std::string>& nameQueue)
{
	while (1)
	{
		std::string name = nameQueue.recieve();
		std::cout << name << std::endl;
	}
}

//void listTree(std::filesystem::path&& rootDir)
//{
//
//}

int main() {
	auto startTime = std::chrono::system_clock::now();
	std::filesystem::path root("C:\\Users\\myles\\Desktop");

	//std::vector<std::filesystem::path> dirsToDo;
	//dirsToDo.push_back(root);

	std::vector<std::string> files;

	MsgQueue<std::filesystem::path> dirQueue;
	MsgQueue<std::string> fileQueue;
	dirQueue.send(std::move(root));

	std::vector<std::future<void>> futures;
	for (int i = 0; i < NUM_THREADS; i++)
	{
		auto ftr = std::async(std::launch::async, listDirectoryServer, std::ref(dirQueue), std::ref(fileQueue));
		//dirsToDo.pop_back();
		futures.push_back(std::move(ftr));
	} 
	futures.push_back(std::async(std::launch::async, printServer, std::ref(fileQueue)));

	try
	{
		while (!futures.empty())
		{
			std::future ftr = std::move(futures.back());
			futures.pop_back();
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

	auto endTime = std::chrono::system_clock::now();
	auto durMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
	std::cout << "\n\nCompleted in " << durMs << std::endl;
}