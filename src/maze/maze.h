#pragma once

#include "grid.h"

#include "algorithm/aldousbroder.h"
#include "algorithm/binarytree.h"
#include "algorithm/growingtree.h"
#include "algorithm/huntkill.h"
#include "algorithm/recursivebacktracker.h"
#include "algorithm/recursivedivision.h"
#include "algorithm/sidewinder.h"
#include "algorithm/trueprims.h"

enum class MazeAlgorithm
{
    AldousBroder,
    BinaryTree,
    GrowingTree,
    HuntKill,
    RecursiveBacktracker,
    RecursiveDivision,
    SideWinder,
    TruePrimes,
    Count
};

class Maze
{
    public:

    Maze() = delete;
    ~Maze() = default;

    explicit Maze(Randomizer& randomizer) : rand(randomizer)
    {
    }

    Grid generate();

    private:

    const int baseGridSize = 20;

    MazeAlgorithm algorithm = MazeAlgorithm::BinaryTree;
    int width = baseGridSize;
    int height = baseGridSize;
    Randomizer& rand;
    float braidRatio = 0.0F;
};