#pragma once

#include "grid.h"
#include "randomizer.h"

namespace AldousBroder
{

    static void use(Grid& grid, Randomizer& rand)
    {

        Cell* cell = &grid.getRandomCell();
        int unvisited = grid.size() - 1;

        while(unvisited > 0)
        {
            auto neighbours = cell->getNeighbours();
            int randIndex = rand.nextInt(static_cast<int>(neighbours.size()) - 1);

            Cell* neighbour = neighbours[randIndex];

            if(neighbour->getLinks().empty())
            {
                cell->link(neighbour);
                unvisited -= 1;
            }

            cell = neighbour;
        }

    }

};