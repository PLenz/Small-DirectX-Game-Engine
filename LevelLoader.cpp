#include "stdafx.h"
#include "LevelLoader.h"


vector<vector<int>> LevelLoader::Load(string filename) {
  std::ifstream input(filename);
  vector<vector<int>>* levelLayout = new vector<vector<int>>;

  for (std::string line; getline(input, line);) {
    vector<int> levelLine;
    for (int i = 0; i < line.size(); ++i) {
      char c = line[i];
      levelLine.push_back(atoi(&c));
    }
    levelLayout->push_back(levelLine);
  }

  return *levelLayout;
}
