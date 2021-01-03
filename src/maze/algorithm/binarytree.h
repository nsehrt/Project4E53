#pragma once

#include "../grid.h"

namespace BinaryTree
{

    static void use(Grid& grid, Randomizer& rand)
    {
        auto& cells = grid.getCells();

        for(auto& cell : cells)
        {
            std::vector<Cell*> neighbours{};
            if(cell.n != nullptr)
            {
                neighbours.push_back(cell.n);
            }
            if(cell.e != nullptr)
            {
                neighbours.push_back(cell.e);
            }

            if(!neighbours.empty())
            {
                int index = rand.nextInt(static_cast<int>(neighbours.size()) - 1);
                Cell* neighbour = neighbours[index];
                if(neighbour != nullptr)
                {
                    cell.link(neighbour);
                }                   
            }


        }
    }

};