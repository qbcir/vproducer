#ifndef __NNXCAM_VPROD_SERIALIZE_HPP_4466__
#define __NNXCAM_VPROD_SERIALIZE_HPP_4466__

#include "type_traits.hpp"
#include <cstdint>

namespace aux {

template<typename T>
size_t serialize(uint8_t* p, const T& val)
{
    for (uint8_t i = 0; i < sizeof(T); i++)
    {
        *p++ = (val >> (i << 3)) & 0xFF;
    }
    return sizeof(T);
}

template<typename T>
size_t deserialize(uint8_t* p, T& val)
{
    val = 0;
    for (uint8_t i = 0; i < sizeof(T); i++)
    {
        val |= (T(*p++) << (i << 3));
    }
    return sizeof(T);
}

template<typename VecT, NNXCAM_ENABLE_IF_IS_VEC(VecT)>
size_t serialize(uint8_t* dst, const VecT &vs) {
    typedef typename VecT::value_type T;
    uint8_t* p = dst;
    p += serialize<size_t>(p, vs.size());
    p += serialize<T>(p, vs.data(), vs.size());
    return dst - p;
}

template<typename T>
size_t serialize(uint8_t* dst, const T* data, size_t num_items)
{
    auto p = dst;
    for (size_t i = 0; i < num_items; i++)
    {
        p += serialize<T>(p, data[num_items]);
    }
    return dst - p;
}

struct Serializable
{
    virtual ~Serializable() {}
    virtual bool serialize(void *p) const = 0;
    virtual bool deserialize(const void *p) = 0;
    virtual size_t size() const { return 0; }
};

template<typename T>
struct is_serializable
{
    static const bool value = std::is_base_of<Serializable, T>::value;
};

#define NNXCAM_ENABLE_IF_IS_SERIALIZABLE_TYPE(T) \
    typename std::enable_if<nnxcam::is_serializable<T>::value, T>::type

#define NNXCAM_ENABLE_IF_IS_SERIALIZABLE(T) NNXCAM_ENABLE_IF_IS_SERIALIZABLE_TYPE(T)* = nullptr

}

#endif /* __NNXCAM_VPROD_SERIALIZE_HPP_4466__ */
