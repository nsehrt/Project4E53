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
    TruePrims,
    Count
};

class Maze
{

    public:

    Maze() = delete;
    ~Maze() = default;

    explicit Maze(int seed, int _width = baseGridSize, int _height = baseGridSize, float _braid = 0.0F)
        : rand(seed), width(_width), height(_height), braidRatio(_braid), grid(_width, _height, rand)
    {

    }

    auto setBraidRatio(float braid) -> void
    {
        if(braid > 1.0F || braid < 0.0F)
        {
            braidRatio = 0.0F;
        }
        else
        {
            braidRatio = braid;
        }
    }

    void generate();

    Grid& getGrid()
    {
        return grid;
    }

    MazeAlgorithm algorithm = MazeAlgorithm::BinaryTree;

    private:

    static const int baseGridSize = 25;
    
    Grid grid;
    int width;
    int height;
    Randomizer rand;
    float braidRatio = 0.0F;
};