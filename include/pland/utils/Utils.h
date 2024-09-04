#pragma once


#include <vector>
namespace land {


template <typename T>
inline bool some(std::vector<T>& vec, T const& val) {
    return std::find(vec.begin(), vec.end(), val) != vec.end();
}


} // namespace land