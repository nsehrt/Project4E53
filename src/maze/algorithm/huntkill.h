#pragma once

#include "grid.h"
#include "randomizer.h"

namespace HuntKill
{

static void use(Grid &grid, Randomizer &rand)
{

    Cell *current = &grid.getRandomCell();

    while (current != nullptr)
    {
        // how many neighbours are not visited
        const auto neighbours = current->getNeighbours();
        std::vector<Cell *> unvisitedNeighbours{};

        for (auto cell : neighbours)
        {
            if (cell->getLinks().empty())
            {
                unvisitedNeighbours.push_back(cell);
            }
        }

        // if there are unvisited neighbours choose a random one
        if (!unvisitedNeighbours.empty())
        {
            int randIndex = rand.nextInt(static_cast<int>(unvisitedNeighbours.size()) - 1);
            Cell *neighbour = unvisitedNeighbours[randIndex];
            current->link(neighbour);
            current = neighbour;
        }
        // hunt down a new start point
        else
        {
            current = nullptr;
            auto &cells = grid.getCells();

            for (Cell &cell : cells)
            {
                std::vector<Cell *> visitedNeighbours{};
                const auto neighbours = cell.getNeighbours();

                for (Cell *n : neighbours)
                {
                    if (!n->getLinks().empty())
                    {
                        visitedNeighbours.push_back(n);
                    }
                }

                if (cell.getLinks().empty() && !visitedNeighbours.empty())
                {
                    current = &cell;
                    int randIndex = rand.nextInt(static_cast<int>(visitedNeighbours.size()) - 1);
                    Cell *neighbour = visitedNeighbours[randIndex];
                    current->link(neighbour);
                }
            }
        }
    }
}
} // namespace HuntKill
