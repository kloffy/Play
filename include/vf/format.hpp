#ifndef VF_FORMAT_HPP_INCLUDED
#define VF_FORMAT_HPP_INCLUDED

#include <boost/format.hpp>

namespace vf {
namespace detail {

std::string format(boost::format& message)
{
    return boost::str(message);
}
 
template<typename T, typename... TArgs>
std::string format(boost::format& message, T&& arg, TArgs&&... args)
{
    return format(message % arg, std::forward<TArgs>(args)...);
}

} // detail

template<typename... TArgs>
std::string format(std::string const& format, TArgs&&... args)
{
    boost::format message{format};
    return detail::format(message, std::forward<TArgs>(args)...);
}

} // vf

#endif // VF_FORMAT_HPP_INCLUDED