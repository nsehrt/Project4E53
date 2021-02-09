#pragma once
#undef max
#include <random>
#include <memory>
#include <numeric>
#include <cassert>
#include <iostream>

class Randomizer
{
    public:

    //from and to are inclusive
    explicit Randomizer(int seed = -1, std::uint32_t max = std::numeric_limits<std::uint32_t>::max())
        :  m_To(max)
    {
        assert(m_To > m_From);

        m_Range = m_To - m_From;
        m_Seed = seed;

        std::random_device rd;
        m_Generator = std::make_unique<std::mt19937>(seed < 0 ? rd() : static_cast<unsigned int>(seed));
        m_Distribution = std::make_unique<std::uniform_int_distribution<std::uint32_t>>(m_From, m_To);
    };

    ~Randomizer() = default;


    Randomizer& operator = (Randomizer &r)
    {
        m_Generator = std::move(r.m_Generator);
        m_Distribution = std::move(r.m_Distribution);
        m_Seed = r.m_Seed;
        m_From = r.m_From;
        m_To = r.m_To;

        return *this;
    }
    

    // return random int in range from-to
    std::uint32_t nextInt() const
    {
        return m_Distribution->operator()(*m_Generator);
    }

    //return uint between min (default 0, inclusive) and max (inclusive)
    std::uint32_t nextInt(int max, int min = 0) const
    {
        if(max == 0) { return 0; }

        assert(max > min);

        float val = nextNormFloat();
        if(val == 1.0F)
        {
            val -= m_Epsilon;
        }    
        
        return static_cast<std::uint32_t>(val * (max-min+1)) + min;
    }

    // return normalized float in range 0.0f-1.0f
    // bigger distribution leads to more precise results
    float nextNormFloat() const
    {
        float r = static_cast<float>(nextInt());
        return r / m_Range;
    }

    // return a random float between min and max(inclusive)
    float nextFloat(float min, float max)
    {
        return min + nextNormFloat() * (max - min);
    }

    private:

    std::unique_ptr<std::mt19937> m_Generator = nullptr;
    std::unique_ptr<std::uniform_int_distribution<std::uint32_t>> m_Distribution;

    std::uint32_t m_From = 0;
    std::uint32_t m_To = 0;
    std::uint32_t m_Range = 0;
    int m_Seed = -1;
    const float m_Epsilon = 0.0001F;
};