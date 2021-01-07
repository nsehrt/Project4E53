#include "maze.h"
#include "../util/serviceprovider.h"

Grid Maze::generate()
{
    Grid grid(width, height, rand);

    switch(algorithm)
    {
        case MazeAlgorithm::BinaryTree: BinaryTree::use(grid, rand); break;
        case MazeAlgorithm::SideWinder: SideWinder::use(grid, rand); break;
        case MazeAlgorithm::AldousBroder: AldousBroder::use(grid, rand); break;
        case MazeAlgorithm::HuntKill: HuntKill::use(grid, rand); break;
        case MazeAlgorithm::RecursiveBacktracker: RecursiveBacktracker::use(grid, rand); break;
        case MazeAlgorithm::TruePrims: TruePrims::use(grid, rand); break;
            // the lambda implements a mix between random cell and last cell
        case MazeAlgorithm::GrowingTree: GrowingTree::use(grid, rand, [&](const auto& active)
                                                          {
                                                              if(rand.nextInt(1) == 1)
                                                              {
                                                                  return active.back();
                                                              }
                                                              else
                                                              {
                                                                  int randIndex = rand.nextInt(static_cast<int>(active.size()) - 1);
                                                                  return active[randIndex];
                                                              }
                                                          }); break;
        case MazeAlgorithm::RecursiveDivision: RecursiveDivision::use(grid, rand); break;
        default: LOG(Severity::Warning, "Undefined maze algorithm!"); break;
    }

    grid.braid(braidRatio);

    return grid;
}