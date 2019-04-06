#pragma once
#include <map>
#include "BoundingVolume.h"
#include "Mesh.h"

class MeshWrapper
{
public:
	MeshWrapper(Mesh* mesh, BoundingVolume* boundingVolume)
	{
		m_mesh = mesh;
		m_boundingVolume = boundingVolume;
	};

	Mesh* m_mesh;
	BoundingVolume* m_boundingVolume;
};
