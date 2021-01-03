#pragma once

#include "../grid.h"

namespace SideWinder
{

    static void use(Grid& grid, Randomizer& rand)
    {

        auto rows = grid.getEachRow();
        std::vector<Cell*> run{};

        for(auto& row : rows)
        {
            for(auto cell : row)
            {
                run.push_back(cell);

                bool atEastern = cell->e == nullptr;
                bool atNorthern = cell->n == nullptr;
                bool shouldClose = atEastern || (!atNorthern && rand.nextInt(1) == 0);

                if(shouldClose)
                {
                    int index = rand.nextInt(static_cast<int>(run.size()) - 1);
                    Cell* randomMember = run[index];
                    if(randomMember->n != nullptr)
                    {
                        randomMember->link(randomMember->n);
                    }
                    run.clear();
                }
                else
                {
                    cell->link(cell->e);
                }

            }
        }


    }

};