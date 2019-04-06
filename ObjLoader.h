#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "BoundingVolume.h"
using namespace std;

class ObjLoader {
public:
  static void Load(
    string filename,
    Vertex* & vertices_out,
    int& vertices_count_out,
    DWORD* & indices_out,
    int& indices_count_out,
    BoundingVolume* & bounding_volume_out);

private:
  static vector<int> split(string);


  ObjLoader() { };


  ~ObjLoader() { };
};
