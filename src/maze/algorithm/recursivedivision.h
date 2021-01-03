#pragma once

#include "../grid.h"

namespace RecursiveDivision {
    static void divide(Grid& grid, Randomizer& rand, int row, int column,
                       int height, int width);
    static void divideVertically(Grid& grid, Randomizer& rand, int row, int column,
                                 int height, int width);
    static void divideHorizontally(Grid& grid, Randomizer& rand, int row,
                                   int column, int height, int width);

    static void use(Grid& grid, Randomizer& rand)
    {
        for(Cell& cell : grid.getCells())
        {
            for(Cell* n : cell.getNeighbours())
            {
                cell.link(n, false);
            }
        }

        divide(grid, rand, 0, 0, grid.rows(), grid.columns());
    }

    static void divide(Grid& grid, Randomizer& rand, int row, int column,
                       int height, int width)
    {
        if(height <= 1 || width <= 1)
        {
            return;
        }

        if(height > width)
        {
            divideHorizontally(grid, rand, row, column, height, width);
        }
        else
        {
            divideVertically(grid, rand, row, column, height, width);
        }
    }

    void divideVertically(Grid& grid, Randomizer& rand, int row, int column,
                          int height, int width)
    {
        int divideEastOf = rand.nextInt(width - 2);
        int passageAt = rand.nextInt(height - 1);

        for(int x = 0; x < height; x++)
        {
            if(x == passageAt)
            {
                continue;
            }

            Cell* cell = grid(column + divideEastOf, row + x);
            cell->unlink(cell->e);
        }

        divide(grid, rand, row, column, height, divideEastOf + 1);
        divide(grid, rand, row, column + divideEastOf + 1, height,
               width - divideEastOf - 1);
    }

    void divideHorizontally(Grid& grid, Randomizer& rand, int row, int column,
                            int height, int width)
    {
        int divideSouthOf = rand.nextInt(height - 2);
        int passageAt = rand.nextInt(width - 1);

        for(int x = 0; x < width; x++)
        {
            if(x == passageAt)
            {
                continue;
            }

            Cell* cell = grid(column + x, row + divideSouthOf);
            cell->unlink(cell->s);
        }

        divide(grid, rand, row, column, divideSouthOf + 1, width);
        divide(grid, rand, row + divideSouthOf + 1, column,
               height - divideSouthOf - 1, width);
    }
};
