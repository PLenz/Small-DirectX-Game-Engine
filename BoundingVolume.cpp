#include "stdafx.h"
#include "BoundingVolume.h"


BoundingVolume::BoundingVolume(float radius) {
  //Used for Camera
  m_type_ = BV_TYPE_SPHERE;
  m_radius_ = radius;
}


BoundingVolume::BoundingVolume(std::vector<Vertex>& vertices, std::vector<DWORD>& indicies) {
  //Used for Game-Objects (Wall, Barrier, etc.)
  m_type_ = BV_TYPE_COMPLEX;
  for (Vertex vertex : vertices) {
    m_vertices_.push_back(XMFLOAT3(vertex.position));
  }

  if (m_vertices_.size() > 0) {
    m_bounding_min_ = XMFLOAT3(m_vertices_[0]);
    m_bounding_max_ = XMFLOAT3(m_vertices_[0]);
    for (int i = 1; i < m_vertices_.size(); ++i) {
      if (m_vertices_[i].x < m_bounding_min_.x)
        m_bounding_min_.x = m_vertices_[i].x;
      if (m_vertices_[i].y < m_bounding_min_.y)
        m_bounding_min_.y = m_vertices_[i].y;
      if (m_vertices_[i].z < m_bounding_min_.z)
        m_bounding_min_.z = m_vertices_[i].z;

      if (m_vertices_[i].x > m_bounding_max_.x)
        m_bounding_max_.x = m_vertices_[i].x;
      if (m_vertices_[i].y > m_bounding_max_.y)
        m_bounding_max_.y = m_vertices_[i].y;
      if (m_vertices_[i].z > m_bounding_max_.z)
        m_bounding_max_.z = m_vertices_[i].z;
    }
  }

  //Actually not used. (Needed for third collision check step)
  m_indices_ = indicies;
}


BoundingVolume::BoundingVolume(std::vector<Vertex>& vertices) {
  //Used for Game-Objects (Wall, Barrier, etc.)
  m_type_ = BV_TYPE_FRUSTUM;
  for (Vertex vertex : vertices) {
    m_vertices_.push_back(XMFLOAT3(vertex.position));
  }

  if (m_vertices_.size() > 0) {
    m_bounding_min_ = XMFLOAT3(m_vertices_[0]);
    m_bounding_max_ = XMFLOAT3(m_vertices_[0]);
    for (int i = 1; i < m_vertices_.size(); ++i) {
      if (m_vertices_[i].x < m_bounding_min_.x)
        m_bounding_min_.x = m_vertices_[i].x;
      if (m_vertices_[i].y < m_bounding_min_.y)
        m_bounding_min_.y = m_vertices_[i].y;
      if (m_vertices_[i].z < m_bounding_min_.z)
        m_bounding_min_.z = m_vertices_[i].z;

      if (m_vertices_[i].x > m_bounding_max_.x)
        m_bounding_max_.x = m_vertices_[i].x;
      if (m_vertices_[i].y > m_bounding_max_.y)
        m_bounding_max_.y = m_vertices_[i].y;
      if (m_vertices_[i].z > m_bounding_max_.z)
        m_bounding_max_.z = m_vertices_[i].z;
    }
  }
}


bool BoundingVolume::Update(XMFLOAT3* position, XMFLOAT4* rotation) {
  if ((position->x != m_position_.x || position->y != m_position_.y || position->z != m_position_.z) ||
    (rotation->x != m_rotation_.x || rotation->y != m_rotation_.y || rotation->z != m_rotation_.z || rotation->w !=
      m_rotation_.w)) {
    m_position_ = XMFLOAT3(*position);
    m_rotation_ = XMFLOAT4(*rotation);

    XMFLOAT3 minTemp, maxTemp;

    if (m_type_ == BV_TYPE_SPHERE) {
      minTemp = XMFLOAT3(-m_radius_, -m_radius_, -m_radius_);
      maxTemp = XMFLOAT3(m_radius_, m_radius_, m_radius_);
      position->y -= 0.5;
    } else if (m_type_ == BV_TYPE_COMPLEX || m_type_ == BV_TYPE_FRUSTUM) {
      minTemp = XMFLOAT3(m_bounding_min_);
      maxTemp = XMFLOAT3(m_bounding_max_);
    }

    if (m_type_ != BV_TYPE_FRUSTUM) {
      m_rotated_vertices_[0] = XMFLOAT3(minTemp.x, minTemp.y, maxTemp.z); // left down front
      m_rotated_vertices_[1] = XMFLOAT3(maxTemp.x, minTemp.y, maxTemp.z); // right down front
      m_rotated_vertices_[2] = XMFLOAT3(maxTemp); // right up front
      m_rotated_vertices_[3] = XMFLOAT3(minTemp.x, maxTemp.y, maxTemp.z); // left up front

      m_rotated_vertices_[4] = XMFLOAT3(minTemp); // left down back
      m_rotated_vertices_[5] = XMFLOAT3(maxTemp.x, minTemp.y, minTemp.z); // right down back
      m_rotated_vertices_[6] = XMFLOAT3(maxTemp.x, maxTemp.y, minTemp.z); // right up back
      m_rotated_vertices_[7] = XMFLOAT3(minTemp.x, maxTemp.y, minTemp.z); // left up back
    } else {
      for (size_t i = 0; i < 8; i++) {
        m_rotated_vertices_[i] = XMFLOAT3(m_vertices_[i]);
      }
    }

    for (int i = 0; i < 8; ++i) {
      XMStoreFloat3(&m_rotated_vertices_[i],
                    XMVector3Rotate(XMLoadFloat3(&m_rotated_vertices_[i]), XMLoadFloat4(rotation)));
      m_rotated_vertices_[i].x += position->x;
      m_rotated_vertices_[i].y += position->y;
      m_rotated_vertices_[i].z += position->z;
    }
    m_min_ = XMFLOAT3(m_rotated_vertices_[0]);
    m_max_ = XMFLOAT3(m_rotated_vertices_[0]);
    for (int i = 1; i < 8; ++i) {
      if (m_rotated_vertices_[i].x < m_min_.x)
        m_min_.x = m_rotated_vertices_[i].x;
      if (m_rotated_vertices_[i].y < m_min_.y)
        m_min_.y = m_rotated_vertices_[i].y;
      if (m_rotated_vertices_[i].z < m_min_.z)
        m_min_.z = m_rotated_vertices_[i].z;

      if (m_rotated_vertices_[i].x > m_max_.x)
        m_max_.x = m_rotated_vertices_[i].x;
      if (m_rotated_vertices_[i].y > m_max_.y)
        m_max_.y = m_rotated_vertices_[i].y;
      if (m_rotated_vertices_[i].z > m_max_.z)
        m_max_.z = m_rotated_vertices_[i].z;
    }

    SetNormals();
    SetOffsets();
  }
  return true;
}


bool BoundingVolume::CullAlongAxes(BoundingVolume* bounding_volume) {
  XMFLOAT3 temp;
  return this->IntersectsAlongAxes(bounding_volume, temp);
}


bool BoundingVolume::IntersectsAlongAxes(BoundingVolume* bounding_volume, XMFLOAT3& out_resolution) {
  if (m_min_.x < bounding_volume->m_max_.x && m_max_.x > bounding_volume->m_min_.x &&
    m_min_.z < bounding_volume->m_max_.z && m_max_.z > bounding_volume->m_min_.z) {
    out_resolution = XMFLOAT3();
    float xleft = m_max_.x - bounding_volume->m_min_.x;
    float xright = m_min_.x - bounding_volume->m_max_.x;
    out_resolution.x = abs(xleft) < abs(xright) ? xleft : xright;

    float zleft = m_max_.z - bounding_volume->m_min_.z;
    float zright = m_min_.z - bounding_volume->m_max_.z;
    out_resolution.z = abs(zleft) < abs(zright) ? zleft : zright;

    if (abs(out_resolution.x) < abs(out_resolution.z)) {
      out_resolution.z = 0.0f;
    } else {
      out_resolution.x = 0.0f;
    }

    return true;
  }
  return false;
}


bool BoundingVolume::CullExactly(BoundingVolume* bounding_volume) {
  XMFLOAT3 temp;
  return this->IntersectsExactly(bounding_volume, temp);
}


bool BoundingVolume::IntersectsExactly(BoundingVolume* other, XMFLOAT3& out_resolution) {
  std::vector<XMFLOAT3> otherVertices;
  other->GetVertices(otherVertices);

  for (int normalIndex = 0; normalIndex < m_normals_.size(); normalIndex++) {
    XMFLOAT3 currentNormal = m_normals_[normalIndex];
    float currentOffset = m_offsets_[normalIndex];
    for (int vectorIndex = 0; vectorIndex < otherVertices.size(); vectorIndex++) {
      XMFLOAT3 currentOtherVector = otherVertices[vectorIndex];
      float projection = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&currentOtherVector), XMLoadFloat3(&currentNormal))) -
        currentOffset;

      if (projection < 0)
        break;
      if (vectorIndex == (otherVertices.size() - 1)) {
        return false;
      }
    }
  }
  return true;
}


void BoundingVolume::GetVertices(std::vector<XMFLOAT3>& vertices) {
  for (XMFLOAT3 elem : m_rotated_vertices_)
    vertices.push_back(XMFLOAT3(elem));
}


// Diese Methode gibt uns die benötigten Kanten zurück,
// welche wir für die 6 Normalen (vorne, hinten, links, rechts, oben, unten) benötigen.
void BoundingVolume::GetEdges(std::vector<XMFLOAT3>& edges) {
  XMFLOAT3 temp;

  XMVECTOR rightDownFrontVector = XMLoadFloat3(&m_rotated_vertices_[1]);
  XMVECTOR rightUpFrontVector = XMLoadFloat3(&m_rotated_vertices_[2]);
  XMVECTOR rightDownBackVector = XMLoadFloat3(&m_rotated_vertices_[5]);
  XMVECTOR rightUpBackVector = XMLoadFloat3(&m_rotated_vertices_[6]);

  XMVECTOR leftDownFrontVector = XMLoadFloat3(&m_rotated_vertices_[0]);
  XMVECTOR leftDownBackVector = XMLoadFloat3(&m_rotated_vertices_[4]);
  XMVECTOR leftUpBackVector = XMLoadFloat3(&m_rotated_vertices_[7]);
  XMVECTOR leftUpFrontVector = XMLoadFloat3(&m_rotated_vertices_[3]);


  // right front edge computation 0
  XMVECTOR rightFrontEdgeVector = XMVectorSubtract(rightUpFrontVector, rightDownFrontVector);
  XMStoreFloat3(&temp, rightFrontEdgeVector);
  edges.push_back(temp);

  // bottom down front edge 1
  XMVECTOR bottomDownFrontEdgeVector = XMVectorSubtract(leftDownFrontVector, rightDownFrontVector);
  XMStoreFloat3(&temp, bottomDownFrontEdgeVector);
  edges.push_back(temp);

  // left down edge 2
  XMVECTOR leftDownEdgeVector = XMVectorSubtract(leftDownFrontVector, leftDownBackVector);
  XMStoreFloat3(&temp, leftDownEdgeVector);
  edges.push_back(temp);

  // left back edge 3 
  XMVECTOR leftBackEdgeVector = XMVectorSubtract(leftUpBackVector, leftDownBackVector);
  XMStoreFloat3(&temp, leftBackEdgeVector);
  edges.push_back(temp);

  // bottom down back edge 4
  XMVECTOR bottomDownBackEdgeVector = XMVectorSubtract(rightDownBackVector, leftDownBackVector);
  XMStoreFloat3(&temp, bottomDownBackEdgeVector);
  edges.push_back(temp);

  // right down edge 5
  XMVECTOR rightDownEdgeVector = XMVectorSubtract(rightDownBackVector, rightDownFrontVector);
  XMStoreFloat3(&temp, rightDownEdgeVector);
  edges.push_back(temp);

  // front up edge 6
  XMVECTOR frontUpEdgeVector = XMVectorSubtract(leftUpFrontVector, rightUpFrontVector);
  XMStoreFloat3(&temp, frontUpEdgeVector);
  edges.push_back(temp);

  // right up edge 7
  XMVECTOR rightUpEdgeVector = XMVectorSubtract(rightUpBackVector, rightUpFrontVector);
  XMStoreFloat3(&temp, rightUpEdgeVector);
  edges.push_back(temp);
}


// Hier bilden wir das Krezprodukt der Kanten, die wir aus der oberen Methode bekommen haben.
// Dadurch erlangen wir die Achsen auf die wir alle Vertices projezieren
void BoundingVolume::SetNormals() {
  m_normals_.clear();

  XMFLOAT3 temp;
  std::vector<XMFLOAT3> edges;
  this->GetEdges(edges);

  // Front normal (right front edge x bottom front edge) (0 x 1) index 0
  XMVECTOR rightFrontEdgeVector = XMLoadFloat3(&edges[0]);
  XMVECTOR bottomFrontEdgeVector = XMLoadFloat3(&edges[1]);
  XMVECTOR frontNormalVector = XMVector3Normalize(XMVector3Cross(rightFrontEdgeVector, bottomFrontEdgeVector));
  XMStoreFloat3(&temp, frontNormalVector);
  m_normals_.push_back(temp);

  // Left normal (left down edge x left back edge) (2 x 3) index 1
  XMVECTOR leftBottomEdgeVector = XMLoadFloat3(&edges[2]);
  XMVECTOR leftBackEdgeVector = XMLoadFloat3(&edges[3]);
  XMVECTOR leftNormalVector = XMVector3Normalize(XMVector3Cross(leftBottomEdgeVector, leftBackEdgeVector));
  XMStoreFloat3(&temp, leftNormalVector);
  m_normals_.push_back(temp);

  // Back normal (left back edge x back bottom edge) (3 x 4) index 2
  XMVECTOR backBottomEdgeVector = XMLoadFloat3(&edges[4]);
  XMVECTOR backNormalVector = XMVector3Normalize(XMVector3Cross(leftBackEdgeVector, backBottomEdgeVector));
  XMStoreFloat3(&temp, backNormalVector);
  m_normals_.push_back(temp);

  // right normal (right down edge x right front edge) (5 x 0) index 3
  XMVECTOR rightDownEdgeVector = XMLoadFloat3(&edges[5]);
  XMVECTOR rightNormalVector = XMVector3Normalize(XMVector3Cross(rightDownEdgeVector, rightFrontEdgeVector));
  XMStoreFloat3(&temp, rightNormalVector);
  m_normals_.push_back(temp);

  // up normal (right up edge x front up edge) (7 x 6) index 4
  XMVECTOR rightUpEdge = XMLoadFloat3(&edges[7]);
  XMVECTOR frontUpEdge = XMLoadFloat3(&edges[6]);
  XMVECTOR upNormalVector = XMVector3Normalize(XMVector3Cross(rightUpEdge, frontUpEdge));
  XMStoreFloat3(&temp, upNormalVector);
  m_normals_.push_back(temp);

  // down normal (front bottom edge x right bottom edge) (1 x 5) index 5
  XMVECTOR downNormalVector = XMVector3Normalize(XMVector3Cross(bottomFrontEdgeVector, rightDownEdgeVector));
  XMStoreFloat3(&temp, downNormalVector);
  m_normals_.push_back(temp);
}


void BoundingVolume::SetOffsets() {
  m_offsets_.clear();
  float offset;

  XMVECTOR rightDownFrontVector = XMLoadFloat3(&m_rotated_vertices_[1]);
  XMVECTOR leftUpBackVector = XMLoadFloat3(&m_rotated_vertices_[7]);

  // Calculate offset for front normal
  offset = XMVectorGetX(XMVector3Dot(rightDownFrontVector, XMLoadFloat3(&m_normals_[FRONT_NORMAL_INDEX])));
  m_offsets_.push_back(abs(offset));

  // Calculate offset for left normal
  offset = XMVectorGetX(XMVector3Dot(leftUpBackVector, XMLoadFloat3(&m_normals_[LEFT_NORMAL_INDEX])));
  m_offsets_.push_back(abs(offset));

  // Calculate offset for back normal
  offset = XMVectorGetX(XMVector3Dot(leftUpBackVector, XMLoadFloat3(&m_normals_[BACK_NORMAL_INDEX])));
  m_offsets_.push_back(abs(offset));

  // Calculate offset for right normal
  offset = XMVectorGetX(XMVector3Dot(rightDownFrontVector, XMLoadFloat3(&m_normals_[RIGHT_NORMAL_INDEX])));
  m_offsets_.push_back(abs(offset));

  // Calculate offset for up normal
  offset = XMVectorGetX(XMVector3Dot(leftUpBackVector, XMLoadFloat3(&m_normals_[UP_NORMAL_INDEX])));
  m_offsets_.push_back(abs(offset));

  // Calculate offset for down normal
  offset = XMVectorGetX(XMVector3Dot(rightDownFrontVector, XMLoadFloat3(&m_normals_[DOWN_NORMAL_INDEX])));
  m_offsets_.push_back(abs(offset));
}
