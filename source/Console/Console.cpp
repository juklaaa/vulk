#include "Console.h"
#include "Console/GlobalVar.h"
#include "Engine/Log.h"
#include <Windows.h>
#include <conio.h>

#include "ConsoleFunction.h"

void Console::run()
{
	thread = std::thread([this]
	{
		std::string line;
		while (!done.test(std::memory_order_relaxed))
		{
			char buffer[1024];
			memset(buffer, 0, sizeof(buffer));
			
			int c = 0;
			if ((c = fgetc(stdin)) != EOF)
			{
				userIsTyping.store(true, std::memory_order_relaxed);
				buffer[0] = (char)c;
				if (!fgets(buffer + 1, sizeof(buffer) - 1, stdin))
				{
					userIsTyping.store(false, std::memory_order_relaxed);
					break;
				}
				userIsTyping.store(false, std::memory_order_relaxed);
			}
			
			line.append(buffer);
			size_t eolPos = 0;
			while ((eolPos = line.find('\n', eolPos)) != std::string::npos)
			{
				std::string userLine = line.substr(0, eolPos);
				line.erase(0, eolPos + 1);
				
				std::lock_guard guard(mutex);
				userInput.push(userLine);
			}
		}
	});
	thread.detach(); // it's ok to detach since there's no way to unblock fgets 
}

void Console::processOnMainThread()
{
	if (!userIsTyping.load(std::memory_order_relaxed))
	{
		while (!bufferedLog.empty())
		{
			auto line = bufferedLog.front();
			bufferedLog.pop();
			print(line);
		}
	}
	
	std::lock_guard guard(mutex);
	while (!userInput.empty())
	{
		auto userLine = userInput.front();
		userInput.pop();
		processLine(userLine);
	}
}

void Console::stop()
{
	done.test_and_set(std::memory_order_relaxed);
}

void Console::print(std::string_view line)
{
	if (!userIsTyping.load(std::memory_order_relaxed))
	{
		std::cout << line << std::endl;
	}
	else
	{
		bufferedLog.push(std::string(line));
	}
}

void Console::processLine(std::string_view line)
{
	size_t pos = line.find('=');
	if (pos != std::string::npos)
	{
		std::string varName(line.substr(0, pos));
		std::string varValue(line.substr(pos + 1));
		auto& vars = GlobalVarRegistry::getSingleton().variables;
		auto it = vars.find(varName);
		if (it != vars.end())
		{
			auto var = it->second;
			var->fromString(varValue);
			std::println("{}={}", varName, var->toString());
		}
		else
		{
			std::println("No variable '{}'", varName);
		}
		std::fflush(stdout);
		return;
	}
	using namespace std::literals;
	auto words = std::views::split(line, " "sv) | std::ranges::to<std::vector<std::string>>();
	if (words.empty())
		return;
	
	auto name = words.front();
	auto& vars = GlobalVarRegistry::getSingleton().variables;
	auto it = vars.find(name);
	if (it != vars.end())
	{
		auto var = it->second;
		std::println("{}={}", name, var->toString());
	}
	else
	{
		auto& functions = ConsoleFunctionRegistry::getSingleton().functions;
		auto itFunc = functions.find(name);
		if (itFunc != functions.end())
		{
			auto consoleFunc = itFunc->second;
			words.erase(words.begin());
			auto result = consoleFunc->func(std::move(words));
			std::println("{}->{}", name, result);
		}
		else
			std::println("No variable/function {}", name);
	}
	std::fflush(stdout);
}
