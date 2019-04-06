#pragma once
#include "Renderer.h"

class Camera
{
    public:
        static XMFLOAT4X4 GetViewMatrix();
        static XMFLOAT4X4 GetProjectionMatrix();
        static void Rotate(int, int);
        static void Update(float);
		static void SetAspectRatio(int, int);
		static void SetCameraPosition(XMFLOAT3 position);
		static XMFLOAT3 GetCameraPosition();
		static void getFrustum(BoundingVolume*& bv);
		static void Setup(Renderer* m_renderer);
		static float Camera::GetPitch();
		static float Camera::GetYaw();

	private:
		Camera() {};
        ~Camera() {};
		static Renderer* m_renderer;
		static BoundingVolume m_boundingCamera;
		static BoundingVolume* m_boundingFrustum;
};
