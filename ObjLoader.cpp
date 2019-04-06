#include "stdafx.h"
#include "ObjLoader.h"
#include <iterator>
#include <iostream>


void ObjLoader::Load(string filename, Vertex*& vertices_out, int& vertices_count_out, DWORD*& indices_out, int& indices_count_out,
                     BoundingVolume* & boundingVolume) {
  std::ifstream input(filename);
  vector<XMFLOAT3> positions;
  vector<XMFLOAT2> texcoord;
  vector<Vertex> verticesVector;
  vector<DWORD> indices;
  int counter = 0;
  for (std::string line; getline(input, line);) {
    std::stringstream ss(line);
    std::istream_iterator<std::string> begin(ss);
    std::istream_iterator<std::string> end;
    std::vector<std::string> vstrings(begin, end);
    std::copy(vstrings.begin(), vstrings.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

    string first = vstrings.at(0);
    if (first == "o") {
      continue;
      // Log::Info("Load Object: " + static_cast<string>(vstrings.at(1)));
    }
    if (first == "v") {
      positions.push_back({std::stof(vstrings.at(1)), std::stof(vstrings.at(2)), std::stof(vstrings.at(3))});
    } else if (first == "vt") {
      texcoord.push_back({std::stof(vstrings.at(1)), 1.0f - std::stof(vstrings.at(2))});
    } else if (first == "f") {
      vector<int> mapping;
      for (size_t i = 1; i <= 3; i++) {
        mapping = split(vstrings.at(i));

        Vertex v = {positions.at(mapping[0] - 1), {0, 0, 0, 0}, texcoord.at(mapping[1] - 1)};
        vector<Vertex>::iterator it = std::find(verticesVector.begin(), verticesVector.end(), v);
        if (it == verticesVector.end()) {
          verticesVector.push_back(v);
          indices.push_back(counter);
          counter++;
        } else {
          int index = std::distance(verticesVector.begin(), it);
          indices.push_back(index);
        }
      }
    }
  }

  Vertex* v = new Vertex[verticesVector.size()];
  for (int i = 0; i < verticesVector.size(); ++i) {
    v[i] = verticesVector[i];
  }
  vertices_out = v;
  vertices_count_out = verticesVector.size();

  DWORD* i = new DWORD[indices.size()];
  for (int j = 0; j < indices.size(); ++j) {
    i[j] = indices[j];
  }
  indices_out = i;
  indices_count_out = indices.size();

  boundingVolume = new BoundingVolume(verticesVector, indices);

}


vector<int> ObjLoader::split(string strToSplit) {
  std::stringstream ss(strToSplit);
  std::string item;
  std::vector<int> splittedStrings;
  while (std::getline(ss, item, '/')) {
    splittedStrings.push_back(std::stof(item));
  }
  return splittedStrings;
}
