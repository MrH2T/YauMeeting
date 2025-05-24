#pragma once
#include <iostream>
class Logger
{
public:
	Logger() = default;
	~Logger() = default;

	void log(const std::string& message);
};

