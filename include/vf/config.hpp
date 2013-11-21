#ifndef VF_CONFIG_HPP_INCLUDED
#define VF_CONFIG_HPP_INCLUDED

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)

#define PLATFORM_LINUX

#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)

#define PLATFORM_APPLE

#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

#define PLATFORM_WIN32

#endif

#endif // VF_CONFIG_HPP_INCLUDED