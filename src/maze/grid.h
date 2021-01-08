#pragma once

#include "cell.h"
#include "../util/randomizer.h"
#include <iostream>

class Grid
{
    using GridPosition = std::pair<int, int>;

    public:

    explicit Grid(Randomizer& _rand) : width(16), height(16), rand(_rand) {}
    explicit Grid(int _width, int _height, Randomizer& _rand) : width(_width), height(_height), rand(_rand)
    {
        reset();
    }

    void reset()
    {
        prepareGrid();
        configureCells();
    }

    virtual void prepareGrid()
    {
        cells.clear();

        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < width; j++)
            {
                cells.emplace_back(j, i);
            }
        }

    }

    virtual void configureCells()
    {
        for(auto& cell : cells)
        {
            const auto[row, column] = cell.getPosition();

            cell.n = (*this)(row, column - 1);
            cell.s = (*this)(row, column + 1);
            cell.w = (*this)(row - 1, column);
            cell.e = (*this)(row + 1, column);
        }
    }

    virtual Cell* operator()(int x, int y)
    {
        if(x < 0 || x > width - 1 || y < 0 || y > height - 1)
            return nullptr;

        int index = y * width + x;
        return &cells[index];
    }

    void solve(GridPosition& start, GridPosition& goal)
    {
        Cell* startCell = (*this)(start.first, start.second);
        Cell* goalCell = (*this)(goal.first, goal.second);


        Distances path = distances.path(goalCell);
        this->setDistances(std::move(path));

    }


    int columns() const
    {
        return width;
    }

    int rows() const
    {
        return height;
    }

    Cell& getRandomCell()
    {
        int x = rand.nextInt(size() - 1);
        return cells[x];
    }

    int size() const
    {
        return width * height;
    }

    std::vector<std::vector<Cell*>> getEachRow()
    {
        std::vector<std::vector<Cell*>> result{};

        for(int i = 0; i < rows(); i++)
        {
            std::vector<Cell*> row{};

            for(int j = 0; j < columns(); j++)
            {
                int index = i * columns() + j;
                row.push_back(&cells[index]);
            }
            result.push_back(std::move(row));
        }

        return std::move(result);
    }

    std::vector<Cell>& getCells()
    {
        return cells;
    }

    void setDistances(Distances d)
    {
        distances = d;
    }

    Distances& getDistances()
    {
        return distances;
    }

    std::vector<Cell*> deadends()
    {
        std::vector<Cell*> d{};

        for(auto& cell : cells)
        {
            if(cell.getLinks().size() == 1)
                d.push_back(&cell);
        }

        return d;
    }

    void braid(float p = 1.0F)
    {
        if(p <= 0.0F) return;

        for(Cell* cell : deadends())
        {
            if(rand.nextNormFloat() > p || cell->getLinks().size() != 1)
            {
                continue;
            }

            const auto neighbours = cell->getNeighbours();
            std::vector<Cell*> vNeighbours{};
            std::vector<Cell*> best{};

            for(Cell* n : neighbours)
            {
                if(!cell->isLinked(n))
                {
                    vNeighbours.push_back(n);
                }
            }

            for(Cell* n : vNeighbours)
            {
                if(n->getLinks().size() == 1)
                {
                    best.push_back(n);
                }
            }

            if(best.empty())
            {
                best = vNeighbours;
            }

            int randIndex = rand.nextInt(static_cast<int>(best.size()) - 1);
            Cell* neighbour = best[randIndex];
            cell->link(neighbour);

        }
    }

    friend std::ostream& operator<<(std::ostream& os, Grid& grid)
    {
        const std::string body = "   ";
        const std::string corner = "+";

        std::cout << "+" << Grid::repeat("---+", grid.columns()) << "\n";

        for(const auto& row : grid.getEachRow())
        {
            std::string top = "|";
            std::string bottom = "+";

            for(const auto cell : row)
            {
                const std::string east_boundary = cell->isLinked(cell->e) ? " " : "|";
                const std::string south_boundary = cell->isLinked(cell->s) ? "   " : "---";

                top += body + east_boundary;
                bottom += south_boundary + corner;
            }

            std::cout << top << "\n" << bottom << "\n";
        }


        return os;
    }

    static std::string repeat(const std::string& input, int n)
    {
        std::string ret;
        ret.reserve(input.size() * n);
        while(n--)
        {
            ret += input;
        }
        return ret;
    }

    private:

    Randomizer& rand;
    int width, height;
    std::vector<Cell> cells;
    Distances distances;
};