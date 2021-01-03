#include "maze.h"

Grid Maze::generate()
{
    return Grid(width, height, rand);
}
