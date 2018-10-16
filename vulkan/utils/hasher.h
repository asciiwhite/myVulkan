#pragma once

using Hash = std::size_t;

inline size_t fnv1a_hash_bytes(const unsigned char * first, size_t count)
{
    static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");
    const size_t fnv_offset_basis = 14695981039346656037ULL;
    const size_t fnv_prime = 1099511628211ULL;
    
    size_t result = fnv_offset_basis;
    for (size_t next = 0; next < count; ++next)
    {	// fold in another byte
        result ^= (size_t)first[next];
        result *= fnv_prime;
    }
    return (result);
}

//using boost::hash_combine
template <class T>
inline void hash_combine(std::size_t& seed, T const& value)
{
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T, typename... Args>
inline auto hash(const T& t, const Args&... args)
{
    auto h = std::hash<T>()(t);
    if constexpr(0 < sizeof...(args))
        hash_combine(h, hash(args...));
    return h;
}

template<typename B, typename E>
inline auto hash_range(B begin, const E& end)
{
    decltype(hash(*begin)) h = 0;
    for (; begin != end; ++begin)
        hash_combine(h, hash(*begin));
    return h;
}

template<typename C>
inline auto hash_range(const C& c)
{
    return hash_range(std::begin(c), std::end(c));
}

namespace std
{
    template<typename T> struct BitwiseHash
    {
        std::size_t operator()(T const& in) const
        {
            return fnv1a_hash_bytes(reinterpret_cast<const unsigned char*>(&in), sizeof(T));        }
    };

    template<typename T> struct hash<vector<T>>
    {
        std::size_t operator()(vector<T> const& in) const
        {
            return hash_range(in);
        }
    };
}

class Hasher
{
public:
    template<typename... Args>
    static Hash hashme(const Args&... args)
    {
        return hash(args...);
    }

    static Hash hashme(const unsigned char* data, size_t size)
    {
        return fnv1a_hash_bytes(data, size);
    }

};
