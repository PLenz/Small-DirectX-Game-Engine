#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class LevelLoader
{
public:
    static vector<vector<int>> Load(string filename);

private:
    LevelLoader() {};
    ~LevelLoader() {};
};