#pragma once

#define NOMINMAX
#include "grid.h"
#include "randomizer.h"

namespace GrowingTree
{
constexpr int MAX_WEIGHT = 999;

template <typename Func> static void use(Grid &grid, Randomizer &rand, Func f)
{
    std::vector<Cell *> active{};
    std::unordered_map<Cell *, int> costs{};

    Cell *start = &grid.getRandomCell();
    active.push_back(start);

    // assign random cost to cells
    for (auto &cell : grid.getCells())
    {
        costs[&cell] = rand.nextInt(GrowingTree::MAX_WEIGHT);
    }

    while (!active.empty())
    {
        // use lambda to select cell
        Cell *cell = f(active);

        // neighbours
        std::vector<Cell *> availableNeighbours{};
        for (Cell *c : cell->getNeighbours())
        {
            if (c->getLinks().empty())
            {
                availableNeighbours.push_back(c);
            }
        }

        if (!availableNeighbours.empty())
        {
            auto it = std::min_element(availableNeighbours.begin(), availableNeighbours.end(),
                                       [&](Cell *a, Cell *b) { return costs[a] < costs[b]; });
            Cell *neighbour = *it;
            cell->link(neighbour);
            active.push_back(neighbour);
        }
        else
        {
            active.erase(std::remove(active.begin(), active.end(), cell), active.end());
        }
    }
}
} // namespace GrowingTree
