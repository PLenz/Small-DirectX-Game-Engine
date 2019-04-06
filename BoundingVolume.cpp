#include "stdafx.h"
#include "BoundingVolume.h"


BoundingVolume::BoundingVolume(float radius) {
  //Used for Camera
  m_type = BV_TYPE_SPHERE;
  m_radius = radius;
}


BoundingVolume::BoundingVolume(std::vector<Vertex>& vertices, std::vector<DWORD>& indicies) {
  //Used for Game-Objects (Wall, Barrier, etc.)
  m_type = BV_TYPE_COMPLEX;
  for (Vertex vertex : vertices) {
    m_vertices.push_back(XMFLOAT3(vertex.position));
  }

  if (m_vertices.size() > 0) {
    m_bounding_min = XMFLOAT3(m_vertices[0]);
    m_bounding_max = XMFLOAT3(m_vertices[0]);
    for (int i = 1; i < m_vertices.size(); ++i) {
      if (m_vertices[i].x < m_bounding_min.x)
        m_bounding_min.x = m_vertices[i].x;
      if (m_vertices[i].y < m_bounding_min.y)
        m_bounding_min.y = m_vertices[i].y;
      if (m_vertices[i].z < m_bounding_min.z)
        m_bounding_min.z = m_vertices[i].z;

      if (m_vertices[i].x > m_bounding_max.x)
        m_bounding_max.x = m_vertices[i].x;
      if (m_vertices[i].y > m_bounding_max.y)
        m_bounding_max.y = m_vertices[i].y;
      if (m_vertices[i].z > m_bounding_max.z)
        m_bounding_max.z = m_vertices[i].z;
    }
  }

  //Actually not used. (Needed for third collision check step)
  m_indices = indicies;
}


BoundingVolume::BoundingVolume(std::vector<Vertex>& vertices) {
  //Used for Game-Objects (Wall, Barrier, etc.)
  m_type = BV_TYPE_FRUSTUM;
  for (Vertex vertex : vertices) {
    m_vertices.push_back(XMFLOAT3(vertex.position));
  }

  if (m_vertices.size() > 0) {
    m_bounding_min = XMFLOAT3(m_vertices[0]);
    m_bounding_max = XMFLOAT3(m_vertices[0]);
    for (int i = 1; i < m_vertices.size(); ++i) {
      if (m_vertices[i].x < m_bounding_min.x)
        m_bounding_min.x = m_vertices[i].x;
      if (m_vertices[i].y < m_bounding_min.y)
        m_bounding_min.y = m_vertices[i].y;
      if (m_vertices[i].z < m_bounding_min.z)
        m_bounding_min.z = m_vertices[i].z;

      if (m_vertices[i].x > m_bounding_max.x)
        m_bounding_max.x = m_vertices[i].x;
      if (m_vertices[i].y > m_bounding_max.y)
        m_bounding_max.y = m_vertices[i].y;
      if (m_vertices[i].z > m_bounding_max.z)
        m_bounding_max.z = m_vertices[i].z;
    }
  }
}


bool BoundingVolume::Update(XMFLOAT3* position, XMFLOAT4* rotation) {
  if ((position->x != m_position.x || position->y != m_position.y || position->z != m_position.z) ||
    (rotation->x != m_rotation.x || rotation->y != m_rotation.y || rotation->z != m_rotation.z || rotation->w !=
      m_rotation.w)) {
    m_position = XMFLOAT3(*position);
    m_rotation = XMFLOAT4(*rotation);

    XMFLOAT3 minTemp, maxTemp;

    if (m_type == BV_TYPE_SPHERE) {
      minTemp = XMFLOAT3(-m_radius, -m_radius, -m_radius);
      maxTemp = XMFLOAT3(m_radius, m_radius, m_radius);
      position->y -= 0.5;
    } else if (m_type == BV_TYPE_COMPLEX || m_type == BV_TYPE_FRUSTUM) {
      minTemp = XMFLOAT3(m_bounding_min);
      maxTemp = XMFLOAT3(m_bounding_max);
    }

    if (m_type != BV_TYPE_FRUSTUM) {
      m_rotatedVertices[0] = XMFLOAT3(minTemp.x, minTemp.y, maxTemp.z); // left down front
      m_rotatedVertices[1] = XMFLOAT3(maxTemp.x, minTemp.y, maxTemp.z); // right down front
      m_rotatedVertices[2] = XMFLOAT3(maxTemp); // right up front
      m_rotatedVertices[3] = XMFLOAT3(minTemp.x, maxTemp.y, maxTemp.z); // left up front

      m_rotatedVertices[4] = XMFLOAT3(minTemp); // left down back
      m_rotatedVertices[5] = XMFLOAT3(maxTemp.x, minTemp.y, minTemp.z); // right down back
      m_rotatedVertices[6] = XMFLOAT3(maxTemp.x, maxTemp.y, minTemp.z); // right up back
      m_rotatedVertices[7] = XMFLOAT3(minTemp.x, maxTemp.y, minTemp.z); // left up back
    } else {
      for (size_t i = 0; i < 8; i++) {
        m_rotatedVertices[i] = XMFLOAT3(m_vertices[i]);
      }
    }

    for (int i = 0; i < 8; ++i) {
      XMStoreFloat3(&m_rotatedVertices[i],
                    XMVector3Rotate(XMLoadFloat3(&m_rotatedVertices[i]), XMLoadFloat4(rotation)));
      m_rotatedVertices[i].x += position->x;
      m_rotatedVertices[i].y += position->y;
      m_rotatedVertices[i].z += position->z;
    }
    m_min = XMFLOAT3(m_rotatedVertices[0]);
    m_max = XMFLOAT3(m_rotatedVertices[0]);
    for (int i = 1; i < 8; ++i) {
      if (m_rotatedVertices[i].x < m_min.x)
        m_min.x = m_rotatedVertices[i].x;
      if (m_rotatedVertices[i].y < m_min.y)
        m_min.y = m_rotatedVertices[i].y;
      if (m_rotatedVertices[i].z < m_min.z)
        m_min.z = m_rotatedVertices[i].z;

      if (m_rotatedVertices[i].x > m_max.x)
        m_max.x = m_rotatedVertices[i].x;
      if (m_rotatedVertices[i].y > m_max.y)
        m_max.y = m_rotatedVertices[i].y;
      if (m_rotatedVertices[i].z > m_max.z)
        m_max.z = m_rotatedVertices[i].z;
    }

    setNormals();
    setOffsets();
  }
  return true;
}


bool BoundingVolume::CullAlongAxes(BoundingVolume* other) {
  XMFLOAT3 temp;
  return this->IntersectsAlongAxes(other, temp);
}


bool BoundingVolume::IntersectsAlongAxes(BoundingVolume* other, XMFLOAT3& resolution) {
  if (m_min.x < other->m_max.x && m_max.x > other->m_min.x &&
    m_min.z < other->m_max.z && m_max.z > other->m_min.z) {
    resolution = XMFLOAT3();
    float xleft = m_max.x - other->m_min.x;
    float xright = m_min.x - other->m_max.x;
    resolution.x = abs(xleft) < abs(xright) ? xleft : xright;

    float zleft = m_max.z - other->m_min.z;
    float zright = m_min.z - other->m_max.z;
    resolution.z = abs(zleft) < abs(zright) ? zleft : zright;

    if (abs(resolution.x) < abs(resolution.z)) {
      resolution.z = 0.0f;
    } else {
      resolution.x = 0.0f;
    }

    return true;
  }
  return false;
}


bool BoundingVolume::CullExactly(BoundingVolume* other) {
  XMFLOAT3 temp;
  return this->IntersectsExactly(other, temp);
}


bool BoundingVolume::IntersectsExactly(BoundingVolume* other, XMFLOAT3& resolution) {
  std::vector<XMFLOAT3> otherVertices;
  other->getVertices(otherVertices);

  for (int normalIndex = 0; normalIndex < m_normals.size(); normalIndex++) {
    XMFLOAT3 currentNormal = m_normals[normalIndex];
    float currentOffset = m_offsets[normalIndex];
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


void BoundingVolume::getVertices(std::vector<XMFLOAT3>& vertices) {
  for (XMFLOAT3 elem : m_rotatedVertices)
    vertices.push_back(XMFLOAT3(elem));
}


// Diese Methode gibt uns die benötigten Kanten zurück,
// welche wir für die 6 Normalen (vorne, hinten, links, rechts, oben, unten) benötigen.
void BoundingVolume::getEdges(std::vector<XMFLOAT3>& edges) {
  XMFLOAT3 temp;

  XMVECTOR rightDownFrontVector = XMLoadFloat3(&m_rotatedVertices[1]);
  XMVECTOR rightUpFrontVector = XMLoadFloat3(&m_rotatedVertices[2]);
  XMVECTOR rightDownBackVector = XMLoadFloat3(&m_rotatedVertices[5]);
  XMVECTOR rightUpBackVector = XMLoadFloat3(&m_rotatedVertices[6]);

  XMVECTOR leftDownFrontVector = XMLoadFloat3(&m_rotatedVertices[0]);
  XMVECTOR leftDownBackVector = XMLoadFloat3(&m_rotatedVertices[4]);
  XMVECTOR leftUpBackVector = XMLoadFloat3(&m_rotatedVertices[7]);
  XMVECTOR leftUpFrontVector = XMLoadFloat3(&m_rotatedVertices[3]);


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
void BoundingVolume::setNormals() {
  m_normals.clear();

  XMFLOAT3 temp;
  std::vector<XMFLOAT3> edges;
  this->getEdges(edges);

  // Front normal (right front edge x bottom front edge) (0 x 1) index 0
  XMVECTOR rightFrontEdgeVector = XMLoadFloat3(&edges[0]);
  XMVECTOR bottomFrontEdgeVector = XMLoadFloat3(&edges[1]);
  XMVECTOR frontNormalVector = XMVector3Normalize(XMVector3Cross(rightFrontEdgeVector, bottomFrontEdgeVector));
  XMStoreFloat3(&temp, frontNormalVector);
  m_normals.push_back(temp);

  // Left normal (left down edge x left back edge) (2 x 3) index 1
  XMVECTOR leftBottomEdgeVector = XMLoadFloat3(&edges[2]);
  XMVECTOR leftBackEdgeVector = XMLoadFloat3(&edges[3]);
  XMVECTOR leftNormalVector = XMVector3Normalize(XMVector3Cross(leftBottomEdgeVector, leftBackEdgeVector));
  XMStoreFloat3(&temp, leftNormalVector);
  m_normals.push_back(temp);

  // Back normal (left back edge x back bottom edge) (3 x 4) index 2
  XMVECTOR backBottomEdgeVector = XMLoadFloat3(&edges[4]);
  XMVECTOR backNormalVector = XMVector3Normalize(XMVector3Cross(leftBackEdgeVector, backBottomEdgeVector));
  XMStoreFloat3(&temp, backNormalVector);
  m_normals.push_back(temp);

  // right normal (right down edge x right front edge) (5 x 0) index 3
  XMVECTOR rightDownEdgeVector = XMLoadFloat3(&edges[5]);
  XMVECTOR rightNormalVector = XMVector3Normalize(XMVector3Cross(rightDownEdgeVector, rightFrontEdgeVector));
  XMStoreFloat3(&temp, rightNormalVector);
  m_normals.push_back(temp);

  // up normal (right up edge x front up edge) (7 x 6) index 4
  XMVECTOR rightUpEdge = XMLoadFloat3(&edges[7]);
  XMVECTOR frontUpEdge = XMLoadFloat3(&edges[6]);
  XMVECTOR upNormalVector = XMVector3Normalize(XMVector3Cross(rightUpEdge, frontUpEdge));
  XMStoreFloat3(&temp, upNormalVector);
  m_normals.push_back(temp);

  // down normal (front bottom edge x right bottom edge) (1 x 5) index 5
  XMVECTOR downNormalVector = XMVector3Normalize(XMVector3Cross(bottomFrontEdgeVector, rightDownEdgeVector));
  XMStoreFloat3(&temp, downNormalVector);
  m_normals.push_back(temp);
}


void BoundingVolume::setOffsets() {
  m_offsets.clear();
  float offset;

  XMVECTOR rightDownFrontVector = XMLoadFloat3(&m_rotatedVertices[1]);
  XMVECTOR leftUpBackVector = XMLoadFloat3(&m_rotatedVertices[7]);

  // Calculate offset for front normal
  offset = XMVectorGetX(XMVector3Dot(rightDownFrontVector, XMLoadFloat3(&m_normals[FRONT_NORMAL_INDEX])));
  m_offsets.push_back(abs(offset));

  // Calculate offset for left normal
  offset = XMVectorGetX(XMVector3Dot(leftUpBackVector, XMLoadFloat3(&m_normals[LEFT_NORMAL_INDEX])));
  m_offsets.push_back(abs(offset));

  // Calculate offset for back normal
  offset = XMVectorGetX(XMVector3Dot(leftUpBackVector, XMLoadFloat3(&m_normals[BACK_NORMAL_INDEX])));
  m_offsets.push_back(abs(offset));

  // Calculate offset for right normal
  offset = XMVectorGetX(XMVector3Dot(rightDownFrontVector, XMLoadFloat3(&m_normals[RIGHT_NORMAL_INDEX])));
  m_offsets.push_back(abs(offset));

  // Calculate offset for up normal
  offset = XMVectorGetX(XMVector3Dot(leftUpBackVector, XMLoadFloat3(&m_normals[UP_NORMAL_INDEX])));
  m_offsets.push_back(abs(offset));

  // Calculate offset for down normal
  offset = XMVectorGetX(XMVector3Dot(rightDownFrontVector, XMLoadFloat3(&m_normals[DOWN_NORMAL_INDEX])));
  m_offsets.push_back(abs(offset));
}
