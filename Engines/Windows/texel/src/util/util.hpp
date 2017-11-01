/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * util.hpp
 *
 *  Created on: Feb 26, 2012
 *      Author: petero
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <cstddef>
#include <cstdint>
#include <string>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <atomic>
#include <cctype>
#include <iomanip>

using U64 = uint64_t;
using S64 = int64_t;
using U32 = uint32_t;
using S32 = int32_t;
using U16 = uint16_t;
using S16 = int16_t;
using S8  = int8_t;
using U8  = uint8_t;

template <typename T, size_t N> char (&ArraySizeHelper(T(&array)[N]))[N];
#define COUNT_OF(array) (sizeof(ArraySizeHelper(array)))

template <typename T> class AlignedAllocator;
/** std::vector with cache line aware allocator. */
template <typename T>
class vector_aligned : public std::vector<T, AlignedAllocator<T>> { };

template <typename T, typename ...Args>
inline std::unique_ptr<T>
make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}


/** Helper class to perform static initialization of a class T. */
template <typename T>
class StaticInitializer {
public:
    StaticInitializer() {
        T::staticInitialize();
    }
};

template <typename T>
T clamp(T val, T min, T max) {
    if (val < min)
        return min;
    else if (val > max)
        return max;
    else
        return val;
}

/** Return integer 2-logarithm of a positive number. */
int floorLog2(U32 x);

// ----------------------------------------------------------------------------

/** Split a string using " " as delimiter. Append words to out. */
inline void
splitString(const std::string& str, std::vector<std::string>& out)
{
    std::string word;
    std::istringstream iss(str, std::istringstream::in);
    while (iss >> word)
        out.push_back(word);
}

/** Convert a string to a number. */
template <typename T>
bool
str2Num(const std::string& str, T& result) {
    std::stringstream ss(str);
    ss >> result;
    return !!ss;
}
#if defined(__linux__) && !defined(__arm__) && !defined(__ANDROID__)
inline bool
str2Num(const std::string& str, int& result) {
    try {
        result = std::stoi(str);
        return true;
    } catch (...) {
        result = 0;
        return false;
    }
}
inline bool
str2Num(const std::string& str, double& result) {
    try {
        result = std::stod(str);
        return true;
    } catch (...) {
        result = 0.0;
        return false;
    }
}
#endif

template <typename T>
bool
hexStr2Num(const std::string& str, T& result) {
    std::stringstream ss;
    ss << std::hex << str;
    ss >> result;
    return !!ss;
}

template <typename T>
inline std::string
num2Str(const T& num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
}

inline std::string
num2Hex(U64 num) {
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << num;
    return ss.str();
}

/** Convert string to lower case. */
inline std::string
toLowerCase(std::string str) {
  for (size_t i = 0; i < str.length(); i++)
    str[i] = std::tolower(str[i]);
  return str;
}

inline bool
startsWith(const std::string& str, const std::string& startsWith) {
    size_t N = startsWith.length();
    if (str.length() < N)
        return false;
    for (size_t i = 0; i < N; i++)
        if (str[i] != startsWith[i])
            return false;
    return true;
}

inline bool
endsWith(const std::string& str, const std::string& endsWith) {
    size_t N = endsWith.length();
    size_t sN = str.length();
    if (sN < N)
        return false;
    for (size_t i = 0; i < N; i++)
        if (str[sN - N + i] != endsWith[i])
            return false;
    return true;
}

/** Return true if vector v contains element e. */
template <typename T>
inline bool
contains(const std::vector<T>& v, const T& e) {
    return std::find(v.begin(), v.end(), e) != v.end();
}

/** Return true if vector v contains element e converted to a string. */
inline bool
contains(const std::vector<std::string>& v, const char* e) {
    return contains(v, std::string(e));
}

inline std::string
trim(const std::string& s) {
    for (int i = 0; i < (int)s.length(); i++) {
        if (!isspace(s[i])) {
            for (int j = (int)s.length()-1; j >= i; j--)
                if (!isspace(s[j]))
                    return s.substr(i, j-i+1);
            return "";
        }
    }
    return "";
}

// ----------------------------------------------------------------------------

/** Helper class for shared data where read/write accesses do not have to be sequentially ordered.
 * This typically generates code as efficient as using a plain T. However using a plain T in this
 * context would invoke undefined behavior, see section 1.10.21 in the C++11 standard. */
template <typename T>
class RelaxedShared {
public:
    RelaxedShared<T>() { }
    RelaxedShared<T>(T value) { set(value); }
    RelaxedShared<T>(const RelaxedShared<T>& r) { set(r.get()); }
    RelaxedShared<T>& operator=(const RelaxedShared<T>& r) { set(r.get()); return *this; }
    RelaxedShared<T>& operator=(const T& t) { set(t); return *this; }
    operator T() const { return get(); }
    T get() const { return data.load(std::memory_order_relaxed); }
    void set(T value) { data.store(value, std::memory_order_relaxed); }
private:
    std::atomic<T> data;
};

#endif /* UTIL_HPP_ */
