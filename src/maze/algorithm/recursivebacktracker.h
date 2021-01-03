#pragma once

#include "../grid.h"
#include <stack>

namespace RecursiveBacktracker
{

    static void use(Grid& grid, Randomizer& rand)
    {
        //start cell on the stack
        std::stack<Cell*> stack{};
        Cell* start = &grid.getRandomCell();
        stack.push(start);

        while(!stack.empty())
        {
            //check how many unvisited neighbours the current cell has
            Cell* current = stack.top();
            const auto neighbours = current->getNeighbours();
            std::vector<Cell*> unvisitedNeighbours{};

            for(auto cell : neighbours)
            {
                if(cell->getLinks().empty())
                {
                    unvisitedNeighbours.push_back(cell);
                }
            }

            //backtrack if no unvisited neighbours
            if(unvisitedNeighbours.empty())
            {
                stack.pop();
            }
            else
            //link random unvisited neighbour and put it on the stack
            {
                int randIndex = rand.nextInt(static_cast<int>(unvisitedNeighbours.size()) - 1);
                Cell* neighbour = unvisitedNeighbours[randIndex];
                current->link(neighbour);
                stack.push(neighbour);
            }

        }

    }
}