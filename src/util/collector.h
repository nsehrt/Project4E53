#pragma once

#include <deque>

template<typename T>
class Collector
{

    public:

    Collector(std::size_t _maxSize = 1024) : maxSize(_maxSize)
    {
       
    }

    ~Collector(){}

    Collector(const Collector& other) = delete;
    Collector& operator=(Collector other) = delete;


    void push_back(const T& item)
    {
        if(data.size() == maxSize)
        {
            data.pop_front();
        }

        data.emplace_back(std::move(item));
    }

    const std::deque<T>& getData() const
    {
        return data;
    }

    size_t size() const
    {
        return data.size();
    }

    void clear()
    {
        data.clear();
    }

    bool empty() const
    {
        return data.empty();
    }

    void fill(T element)
    {
        clear();

        for(std::size_t i = 0; i < maxSize; i++)
        {
            push_back(element);
        }
    }

    auto begin() const
    {
        return data.begin();
    }

    auto end() const
    {
        return data.end();
    }

    private:

    std::size_t maxSize;
    std::deque<T> data;
};