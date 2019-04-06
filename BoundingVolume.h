#pragma once
#define BV_TYPE_SPHERE 0
#define BV_TYPE_COMPLEX 1
#define BV_TYPE_FRUSTUM 2

#define FRONT_NORMAL_INDEX 0
#define LEFT_NORMAL_INDEX 1
#define BACK_NORMAL_INDEX 2
#define RIGHT_NORMAL_INDEX 3
#define UP_NORMAL_INDEX 4
#define DOWN_NORMAL_INDEX 5

#include <limits>


class BoundingVolume
{
public:
	BoundingVolume(float radius);
	BoundingVolume(std::vector<Vertex> &vertices, std::vector<DWORD> &indices);
	BoundingVolume(std::vector<Vertex> &vertices);
    ~BoundingVolume() {};

	bool CullAlongAxes(BoundingVolume* other);
	bool CullExactly(BoundingVolume* other);
    bool IntersectsAlongAxes(BoundingVolume* other, XMFLOAT3 &resolution);
	bool IntersectsExactly(BoundingVolume* other, XMFLOAT3 &resolution);
	bool Update(XMFLOAT3* position, XMFLOAT4* rotation );

protected:
	void getVertices(std::vector<XMFLOAT3>& vertices);
	void setNormals();
	void setOffsets();
	void getEdges(std::vector<XMFLOAT3>& edges);

private:
	int m_type;
	XMFLOAT3 m_min;
	XMFLOAT3 m_max;
    std::vector<XMFLOAT3> m_vertices;
	std::vector<DWORD> m_indices;
	XMFLOAT3 m_rotatedVertices[8];
	float m_radius;
	XMFLOAT4 m_rotation = { FLT_MAX, FLT_MAX ,FLT_MAX ,FLT_MAX };
	XMFLOAT3 m_position = { FLT_MAX, FLT_MAX ,FLT_MAX  };
	XMFLOAT3 m_bounding_min;
	XMFLOAT3 m_bounding_max;
	std::vector<XMFLOAT3> m_normals;
	std::vector<float> m_offsets;
};