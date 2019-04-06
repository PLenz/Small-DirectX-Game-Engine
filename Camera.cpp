#include "stdafx.h"
#include "BoundingVolume.h"
#include "Camera.h"
#include "Keyboard.h"

Renderer* Camera::p_renderer_;
BoundingVolume Camera::m_bounding_camera_ = BoundingVolume(0.15f); // Radius = 0.15 -> Breite = 0.3
BoundingVolume* Camera::m_bounding_frustum_;

// X,Y,Z Coordinates to set the direction we are looking at. Always Z-1 from m_cameraPosition Z
const XMFLOAT3 c_forwardVector = {0.0f, 0.0f, -1.0f};
const XMFLOAT4 c_cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f); // the worlds up vector
const float c_cameraSpeed = 0.005f;
const float c_playerHeight = 1.8f;
const float c_fovAngleY = 45.0f * (XM_PI / 180.0f);
const float c_nearZ = 0.1f;
const float c_farZ = 200.0f;

XMFLOAT3 m_cameraTarget = XMFLOAT3();
float m_pitch = 0;
float m_yaw = 0;
float aspectRatio = 1;

// X,Y,Z Coordinates for the position of the camera
XMFLOAT3 m_cameraPosition = XMFLOAT3(10.0f, c_playerHeight, 14.0f);


// X,Y,Z Coordinates of the camera target describing the point in space our camera is looking at. Needs to be X,Y,Z of m_cameraPosition - X,Y,Z of forwardVector
XMFLOAT3 getCameraTarget(XMVECTOR& rotatedForwardVector) {
  XMFLOAT3 forward;
  XMStoreFloat3(&forward, rotatedForwardVector);
  m_cameraTarget = XMFLOAT3(m_cameraPosition.x + forward.x, m_cameraPosition.y + forward.y,
                            m_cameraPosition.z + forward.z);
  return m_cameraTarget;
}


void Camera::SetAspectRatio(int width, int height) {
  aspectRatio = (float)width / (float)height;
}


void Camera::SetCameraPosition(XMFLOAT3 position) {
  m_cameraPosition = position;
  Rotate(1, 1);
}


XMFLOAT3 Camera::GetCameraPosition() {
  return XMFLOAT3(m_cameraPosition);
}


float Camera::GetPitch() {
  return m_pitch;
}


float Camera::GetYaw() {
  return m_yaw;
}


void Camera::Setup(Renderer* renderer) {
  p_renderer_ = renderer;
}


XMFLOAT4X4 Camera::GetViewMatrix() {
  XMVECTOR cPos = XMLoadFloat3(&m_cameraPosition);

  XMVECTOR quaternionRotation = XMQuaternionMultiply(XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0),
                                                     XMQuaternionIdentity());
  XMVECTOR forwardVector = XMVector3Rotate(XMLoadFloat3(&c_forwardVector), quaternionRotation);
  XMVECTOR cameraTarget = XMLoadFloat3(&getCameraTarget(forwardVector));

  XMVECTOR cUp = XMLoadFloat4(&c_cameraUp);

  XMFLOAT4X4 m_cameraViewMat = {}; // this will store our view matrix
  XMStoreFloat4x4(&m_cameraViewMat, XMMatrixLookAtRH(cPos, cameraTarget, cUp));
  return m_cameraViewMat;
}


XMFLOAT4X4 Camera::GetProjectionMatrix() {
  XMFLOAT4X4 m_cameraProjMat = {}; // this will store our projection matrix
  XMStoreFloat4x4(&m_cameraProjMat, XMMatrixPerspectiveFovRH(c_fovAngleY, aspectRatio, c_nearZ, c_farZ));
  return m_cameraProjMat;
}


void Camera::GetFrustum(BoundingVolume*& bounding_volume) {
  if (m_bounding_frustum_ == nullptr) {
    float nearHeight = 2 * tan(c_fovAngleY) * c_nearZ;
    float farHeight = 2 * tan(c_fovAngleY) * c_farZ;
    float nearWidth = nearHeight * aspectRatio;
    float farWidth = farHeight * aspectRatio;

    std::vector<Vertex> frustumModel;

    //left down front
    frustumModel.push_back(Vertex({XMFLOAT3(-nearWidth / 2, -nearHeight / 2, -c_nearZ), {0, 0, 0, 0}, {0, 0}}));
    //right down front
    frustumModel.push_back(Vertex({XMFLOAT3(nearWidth / 2, -nearHeight / 2, -c_nearZ), {0, 0, 0, 0}, {0, 0}}));
    //right up front
    frustumModel.push_back(Vertex({XMFLOAT3(nearWidth / 2, nearHeight / 2, -c_nearZ), {0, 0, 0, 0}, {0, 0}}));
    //left up front
    frustumModel.push_back(Vertex({XMFLOAT3(-nearWidth / 2, nearHeight / 2, -c_nearZ), {0, 0, 0, 0}, {0, 0}}));
    //left down back
    frustumModel.push_back(Vertex({XMFLOAT3(-farWidth / 2, -farHeight / 2, -c_farZ), {0, 0, 0, 0}, {0, 0}}));
    //right down back
    frustumModel.push_back(Vertex({XMFLOAT3(farWidth / 2, -farHeight / 2, -c_farZ), {0, 0, 0, 0}, {0, 0}}));
    //right up back
    frustumModel.push_back(Vertex({XMFLOAT3(farWidth / 2, farHeight / 2, -c_farZ), {0, 0, 0, 0}, {0, 0}}));
    //left up back
    frustumModel.push_back(Vertex({XMFLOAT3(-farWidth / 2, farHeight / 2, -c_farZ), {0, 0, 0, 0}, {0, 0}}));

    m_bounding_frustum_ = new BoundingVolume(frustumModel);
  }
  bounding_volume = m_bounding_frustum_;
}


void Camera::Rotate(int yaw, int pitch) {
  // Scale to slow the camera movement down
  float scale = 0.003f;

  m_pitch += -pitch * scale;
  m_yaw += -yaw * scale;

  //85°
  float border = 85.0f / (180.0f / XM_PI);

  if (m_pitch > border) {
    m_pitch = border;
  }

  if (m_pitch < -border) {
    m_pitch = -border;
  }
}


void Camera::Update(float delta) {
  XMFLOAT3 direction = {.0f, .0f, .0f};

  if (Keyboard::IsPressed(0x41))
    direction.x -= c_cameraSpeed * delta; // A-Key
  if (Keyboard::IsPressed(0x44))
    direction.x += c_cameraSpeed * delta; // D-Key
  if (Keyboard::IsPressed(0x53))
    direction.z += c_cameraSpeed * delta; // S-Key
  if (Keyboard::IsPressed(0x57))
    direction.z -= c_cameraSpeed * delta; // W-Key


  if (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f)
    return;

  XMVECTOR quaternionRotation = XMQuaternionMultiply(XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0),
                                                     XMQuaternionIdentity());
  XMVECTOR rotatedForwardVector = XMVector3Rotate(XMLoadFloat3(&c_forwardVector), quaternionRotation);
  XMFLOAT3 cameraTarget = getCameraTarget(rotatedForwardVector);

  XMVECTOR directionVector = XMLoadFloat3(&direction);
  XMVECTOR rotatedDirection = XMVector3Rotate(directionVector, quaternionRotation);

  XMFLOAT3 resolution;

  XMFLOAT3 desiredPosition;
  XMStoreFloat3(&desiredPosition, XMLoadFloat3(&m_cameraPosition) + rotatedDirection);
  XMFLOAT4 desiredRotation;
  XMStoreFloat4(&desiredRotation, quaternionRotation);
  m_bounding_camera_.Update(&desiredPosition, &desiredRotation);

  if (p_renderer_->Intersects(m_bounding_camera_, resolution)) {
    rotatedDirection -= XMLoadFloat3(&resolution);
  }

  XMStoreFloat3(&m_cameraPosition, XMLoadFloat3(&m_cameraPosition) + rotatedDirection);
  m_cameraPosition.y = c_playerHeight;


  XMVECTOR camPosVector = XMLoadFloat3(&m_cameraPosition);
  XMStoreFloat3(&m_cameraTarget, camPosVector + rotatedForwardVector);
}
