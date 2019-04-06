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


class BoundingVolume {
public:
  BoundingVolume(float radius);
  BoundingVolume(std::vector<Vertex>& vertices, std::vector<DWORD>& indices);
  BoundingVolume(std::vector<Vertex>& vertices);


  ~BoundingVolume() {};


  bool CullAlongAxes(BoundingVolume* bounding_volume);
  bool CullExactly(BoundingVolume* bounding_volume);
  bool IntersectsAlongAxes(BoundingVolume* bounding_volume, XMFLOAT3& out_resolution);
  bool IntersectsExactly(BoundingVolume* other, XMFLOAT3& out_resolution);
  bool Update(XMFLOAT3* position, XMFLOAT4* rotation);

protected:
  void GetVertices(std::vector<XMFLOAT3>& vertices);
  void SetNormals();
  void SetOffsets();
  void GetEdges(std::vector<XMFLOAT3>& edges);

private:
  int m_type_;
  float m_radius_;

  XMFLOAT3 m_min_;
  XMFLOAT3 m_max_;

  std::vector<XMFLOAT3> m_vertices_;
  std::vector<DWORD> m_indices_;
  XMFLOAT4 m_rotation_ = {FLT_MAX, FLT_MAX,FLT_MAX,FLT_MAX};
  XMFLOAT3 m_position_ = {FLT_MAX, FLT_MAX,FLT_MAX};

  XMFLOAT3 m_rotated_vertices_[8];
  XMFLOAT3 m_bounding_min_;
  XMFLOAT3 m_bounding_max_;
  std::vector<XMFLOAT3> m_normals_;
  std::vector<float> m_offsets_;
};
