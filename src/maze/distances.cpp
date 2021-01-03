#include "distances.h"
#include "cell.h"

Distances Distances::path(Cell* goal)
{
    Cell* current = goal;

    Distances breadcrumbs{ root };
    breadcrumbs.set(current, distances[goal]);

    while(current != root)
    {
        for(Cell* neighbour : current->getLinks())
        {
            if(distances[neighbour] < distances[current])
            {
                breadcrumbs.set(neighbour, distances[neighbour]);
                current = neighbour;
            }
        }
    }

    return breadcrumbs;
}

std::pair<Cell*, int> Distances::maxPath()
{
    int maxDistance = 0;
    Cell* maxCell = root;

    for(const auto& [cell, distance] : distances)
    {
        if(distance > maxDistance)
        {
            maxCell = cell;
            maxDistance = distance;
        }
    }

    return{ maxCell, maxDistance };
}
