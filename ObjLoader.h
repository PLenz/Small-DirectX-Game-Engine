#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "BoundingVolume.h"
using namespace std;

class ObjLoader
{
public:
	static void Load(
		string filename, 
		Vertex* & outVertices, 
		int& vcount, 
		DWORD* & outIndices, 
		int& icount, 
		BoundingVolume* &boundingBox);

private:
	static vector<int>split(string);
	ObjLoader()
	{
	};

	~ObjLoader()
	{
	};
};