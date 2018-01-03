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
inline void hash_combine(std::size_t& seed, T const& v)
{
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
    template<typename T> struct hash<vector<T>>
    {
        std::size_t operator()(vector<T> const& in) const
        {
            size_t size = in.size();
            size_t seed = 0;
            for (size_t i = 0; i < size; i++)
                //Combine the hash of the current vector with the hashes of the previous ones
                hash_combine(seed, in[i]);
            return seed;
        }
    };

    template<typename T> struct BitwiseHash
    {
        std::size_t operator()(T const& in) const
        {
            return fnv1a_hash_bytes(reinterpret_cast<const unsigned char*>(&in), sizeof(T));        }
    };

    template<>
    struct hash<PipelineSettings> : public BitwiseHash<PipelineSettings>
    {};

    template<>
    struct hash<VkPipelineShaderStageCreateInfo> : public BitwiseHash<VkPipelineShaderStageCreateInfo>
    {};
}

class Hasher
{
public:
    template<typename T>
    void add(T& value)
    {
        hash_combine(m_seed, value);
    }

    void add(const unsigned char* data, size_t size)
    {
        const auto dataHash = fnv1a_hash_bytes(data, size);
        hash_combine(m_seed, dataHash);
    }

    Hash get() const
    {
        return m_seed;
    }

private:
    Hash m_seed = 0;
};
