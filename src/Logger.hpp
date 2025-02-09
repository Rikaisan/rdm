#pragma once
#define INFO_TAG "\033[92m[INFO]\033[m "
#define ERR_TAG "\033[91m[ERROR]\033[m "
#define WARN_TAG "\033[93m[WARNING]\033[m "
#define DEBUG_TAG "\033[95m[DEBUG]\033[m "

#ifndef _NLOG
#include <iostream>
#define LOG(x) do {std::cout << x << std::endl;} while(0)
#define LOG_INFO(x) do {std::cout << INFO_TAG << x << std::endl;} while(0)
#define LOG_CUSTOM(name, x) do {std::cout << "\033[94m[" << name << "]\033[m " << x << std::endl;} while(0)
#define LOG_CUSTOM_INFO(name, x) do {std::cout << "\033[94m[" << name << "]" << INFO_TAG << x << std::endl;} while(0)
#define LOG_SEP() do {std::cout << "+------------------------------------------------+" << std::endl;} while(0)
#else
#define LOG(x)
#define LOG_INFO(x)
#define LOG_CUSTOM(name, x)
#define LOG_CUSTOM_INFO(name, x)
#define LOG_SEP()
#endif

#if defined(_DEBUG) && ! defined(_NLOG)
#define LOG_ERR(x) do {std::cerr << ERR_TAG << x << std::endl;} while(0)
#define LOG_WARN(x) do {std::cerr << WARN_TAG << x << std::endl;} while(0)
#define LOG_DEBUG(x) do {std::cerr << DEBUG_TAG << x << std::endl;} while(0)
#define LOG_CUSTOM_ERR(name, x) do {std::cerr << "\033[94m[" << name << "]" << ERR_TAG << x << std::endl;} while(0)
#define LOG_CUSTOM_WARN(name, x) do {std::cerr << "\033[94m[" << name << "]" << WARN_TAG << x << std::endl;} while(0)
#define LOG_CUSTOM_DEBUG(name, x) do {std::cerr << "\033[94m[" << name << "]" << DEBUG_TAG << x << std::endl;} while(0)
#else
#define LOG_ERR(x)
#define LOG_WARN(x)
#define LOG_DEBUG(x)
#define LOG_CUSTOM_ERR(name, x)
#define LOG_CUSTOM_WARN(name, x)
#define LOG_CUSTOM_DEBUG(name, x)
#endif