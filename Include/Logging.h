#pragma once

#include <windows.h>
#include <sstream>
#include <iomanip>
#include <chrono>

class Logging
{
public:
	template<typename... Args>
	static void DEBUG_LOG(Args&&... args)
	{
		std::wostringstream ss;
		ss << L"\n";
		(ss << ... << std::forward<Args>(args));
		ss << L"\n";
		OutputDebugStringW(ss.str().c_str());
	}

	static void DEBUG_BAR(size_t a, size_t total, int inc = 60)
	{
		if (total < inc) inc = total;
		size_t div = total / inc;
		if (a % div == 0) {
			auto curr = std::chrono::high_resolution_clock::now();
			auto inctime = std::chrono::duration_cast<std::chrono::microseconds>(curr - start).count();
			inctime /= (a / div) + 1;
			
			int rem = inc - a / div;
			size_t t = inctime * rem;
			double s = (float)(t % 60000000) / 1000000;

			std::wstringstream ss;
			ss << L"[" << std::wstring(a / div, '#') << std::wstring(rem, '-') << L"] " << a << L"/" << total;
			ss << " ETA: " << t / 60000000 << L":" << std::fixed << std::setfill(L'0') << std::setw(5) << std::setprecision(2) << s << L"\n";
			OutputDebugStringW(ss.str().c_str());
		}
	}
	static void DEBUG_START()
	{
		start = std::chrono::high_resolution_clock::now();
	}

private:
	inline static std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
};