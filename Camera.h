#pragma once
#include "Renderer.h"

class Camera {
public:
  static XMFLOAT4X4 GetViewMatrix();
  static XMFLOAT4X4 GetProjectionMatrix();
  static void Rotate(int, int);
  static void Update(float);
  static void SetAspectRatio(int, int);
  static void SetCameraPosition(XMFLOAT3 position);
  static XMFLOAT3 GetCameraPosition();
  static void GetFrustum(BoundingVolume*& bounding_volume);
  static void Setup(Renderer* renderer);
  static float Camera::GetPitch();
  static float Camera::GetYaw();

private:
  Camera() {};


  ~Camera() {};


  static Renderer* p_renderer_;
  static BoundingVolume m_bounding_camera_;
  static BoundingVolume* m_bounding_frustum_;
};
