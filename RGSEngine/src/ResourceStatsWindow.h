#pragma once

#include "imgui.h"
#include <string>
#include <vector>

struct ResourceStat
{
    std::string name;
    std::string type;
    int references;
    std::string path;
};

class ResourceStatsWindow
{
public:
    void Draw(bool* pOpen);
    void Refresh();

private:
    std::vector<ResourceStat> stats;
    int sortBy = 0; 
    bool descendingOrder = false;
};