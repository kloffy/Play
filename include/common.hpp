#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED

// Standard Library
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>

namespace std {

template<typename T, typename... TArgs>
std::unique_ptr<T> make_unique(TArgs&&... args)
{
    return std::unique_ptr<T>{new T(std::forward<TArgs>(args)...)};
}

}

// Boost
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace fs = boost::filesystem;
namespace po = boost::program_options;

// Custom
#include "vf/ext/al.hpp"
#include "vf/ext/av.hpp"
#include "vf/format.hpp"

namespace al = vf::ext::al;
namespace av = vf::ext::av;
namespace swr = vf::ext::swr;
namespace sws = vf::ext::sws;

#endif // COMMON_HPP_INCLUDED