#pragma once

using Hash = std::size_t;

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

    template<>
    struct hash<PipelineSettings> : public _Bitwise_hash<PipelineSettings>
    {};

    template<>
    struct hash<VkPipelineShaderStageCreateInfo> : public _Bitwise_hash<VkPipelineShaderStageCreateInfo>
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
        const auto dataHash = std::_Hash_seq(data, size);
        hash_combine(m_seed, dataHash);
    }

    Hash get() const
    {
        return m_seed;
    }

private:
    Hash m_seed = 0;
};
