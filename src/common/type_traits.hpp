#ifndef __NNXCAM_VPROD_TYPE_TRAITS_HPP_23667__
#define __NNXCAM_VPROD_TYPE_TRAITS_HPP_23667__

#include <functional>
#include <type_traits>
#include <vector>

namespace nnxcam {

template<typename T>
struct is_vec
{
    static const bool value = false;
};

template<typename T>
struct is_vec<std::vector<T>>
{
    static const bool value = true;
};

template<typename T>
struct is_func
{
    static const bool value = false;
};

template<typename F>
struct is_func<std::function<F>>
{
    static const bool value = true;
};

}

#define NNXCAM_ENABLE_IF_IS_INT_TYPE(T) \
    typename std::enable_if<std::is_integral<T>::value && !std::is_enum<T>::value, T>::type

#define NNXCAM_ENABLE_IF_IS_VEC_TYPE(T) \
    typename std::enable_if<::nnxcam::is_vec<T>::value, T>::type

#define NNXCAM_ENABLE_IF_IS_FUNC_TYPE(T) \
    typename std::enable_if<::nnxcam::is_func<T>::value, T>::type

#define NNXCAM_ENABLE_IF_IS_INT(T) NNXCAM_ENABLE_IF_IS_INT_TYPE(T)* = nullptr
#define NNXCAM_ENABLE_IF_IS_VEC(T) NNXCAM_ENABLE_IF_IS_VEC_TYPE(T)* = nullptr
#define NNXCAM_ENABLE_IF_IS_FUNC(T) NNXCAM_ENABLE_IF_IS_FUNC_TYPE(T)* = nullptr

#endif /* __NNXCAM_VPROD_TYPE_TRAITS_HPP_23667__ */
