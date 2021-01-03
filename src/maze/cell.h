#pragma once

#include "distances.h"
#include <vector>
#include <algorithm>

class Cell
{
    public:

    explicit Cell(int x, int y) : n(nullptr), e(nullptr), s(nullptr), w(nullptr), 
        xPos(x), yPos(y)
    {
    }

    void link(Cell* cell, bool bidirectional = true)
    {
        if(std::find(links.begin(), links.end(), cell) == links.end())
        {
            links.push_back(cell);
        }

        if(bidirectional)
        {
            cell->link(this, false);
        }
    }

    void unlink(Cell* cell, bool bidirectional = true)
    {
        if(cell == nullptr) { return; }

        const auto position = std::find(links.begin(), links.end(), cell);

        if(position != links.end())
        {
            links.erase(position);
        }

        if(bidirectional)
        {
            cell->unlink(this, false);
        }
    }

    [[nodiscard]] const auto& getLinks() const
    {
        return links;
    }

    bool isLinked(const Cell* cell) const
    {
       return std::find(links.begin(), links.end(), cell) != links.end();
    }

    [[nodiscard]]  std::pair<int, int> getPosition() const
    {
        return { xPos, yPos };
    }

    [[nodiscard]] std::vector<Cell*> getNeighbours() const
    {
        std::vector<Cell*> neighbours{};

        if(n != nullptr) { neighbours.push_back(n); }
        if(e != nullptr) { neighbours.push_back(e); }
        if(s != nullptr) { neighbours.push_back(s); }
        if(w != nullptr) { neighbours.push_back(w); }

        return neighbours;
    }

    Distances distances()
    {
        Distances weights(this);
        std::vector<Cell*> pending{ this };

        std::vector<Cell*> frontiers{ this };

        while(!pending.empty())
        {
            std::sort(pending.begin(), pending.end(), [](const Cell* a, const Cell* b)
                                         {
                                             return a->weight > b->weight;
                                         });
            Cell* cell = pending[0];
            pending.erase(std::remove(pending.begin(), pending.end(), cell), pending.end());

            const auto& neighbours = cell->getLinks();

            for(Cell* c : neighbours)
            {
                int totalWeight = weights.get(cell) + c->weight;

                if(weights.get(c) == -1 || totalWeight < weights.get(c))
                {
                    pending.push_back(c);
                    weights.set(c, totalWeight);
                }
            }
        }

        //while(!frontiers.empty())
        //{
        //    std::vector<Cell*> newFrontiers{};

        //    for(Cell* cell : frontiers)
        //    {
        //        for(Cell* link : cell->links)
        //        {
        //            if(weights.exist(link))
        //                continue;

        //            dist.set(link, dist.get(cell) + 1);
        //            newFrontiers.push_back(link);
        //        }
        //    }

        //    frontiers = newFrontiers;
        //}

        return weights;
    }

    public:

    Cell* n, * e, * s, * w;
    int weight = 1;

    private:

    int xPos, yPos;
    std::vector<Cell*> links{};
};