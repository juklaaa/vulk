#pragma once
#include "Common.h"

class Console
{
public:
	
	void run();
	void processOnMainThread();
	void stop();
	
	void print(std::string_view line);
	
private:
	
	void processLine(std::string_view line);
	
	std::thread thread;
	std::mutex mutex;
	std::atomic_flag done;
	std::queue<std::string> userInput;
	std::atomic<bool> userIsTyping = false;
	std::queue<std::string> bufferedLog;
};
