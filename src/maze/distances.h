#pragma once
#include <unordered_map>

class Cell;

class Distances
{
    public:
    explicit Distances(Cell* cell) : root(cell)
    {
        distances[cell] = 0;
    }

    explicit Distances() : root(nullptr)
    {
        
    }

    int get(Cell* cell)
    {
        if(distances.find(cell) == distances.end()) { return -1; }
        return distances[cell];
    }

    void set(Cell* cell, int dist)
    {
        distances[cell] = dist;
    }

    bool exist(Cell* cell)
    {
        return distances.find(cell) != distances.end();
    }

    Distances path(Cell* goal);
    std::pair<Cell*, int> maxPath();

    int maxValue()
    {
        int max = 0;

        for(auto& [cell, dist] : distances)
        {
            if(dist > max)
            {
                max = dist;
            }
        }

        return max;
    }

    private:
    Cell* root;
    std::unordered_map<Cell*, int> distances;
};