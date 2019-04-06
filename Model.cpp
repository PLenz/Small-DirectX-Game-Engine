#include "stdafx.h"
#include "Model.h"
#include "Camera.h"

Model::Model(std::wstring objPath, std::wstring texturePath, XMFLOAT3 position, XMFLOAT4 rotation, bool solid)
{
	m_position = position;
	m_rotation = rotation;
	m_solid = solid;
	m_objPath = objPath;
	m_texturePath = texturePath;
	MeshCache::GetMesh(m_objPath, m_texturePath, m_mesh);
	m_isCulled = true;
}

bool Model::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator)
{
	MeshCache::SetDevice(device);
	m_mesh->LoadResources(device, commandList, commandAllocator);
	MeshCache::SetBoundingVolume(m_objPath, m_texturePath);
	return true;
}

bool Model::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList> &commandList, UINT8* constantBufferGPUAddress, D3D12_GPU_VIRTUAL_ADDRESS constantBufferUploadHeap)
{
	// Wird mit jedem Frame aufgerufen.

	// update constant buffer
	// create the wvp matrix and store in constant buffer
	XMMATRIX viewMat = XMLoadFloat4x4(&Camera::GetViewMatrix()); // load view matrix
	XMMATRIX projMat = XMLoadFloat4x4(&Camera::GetProjectionMatrix()); // load projection matrix
	XMMATRIX rotatMat = XMMatrixRotationQuaternion(XMLoadFloat4(&m_rotation));

	XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat3(&m_position));
	XMMATRIX worldMat = rotatMat * translationMat;

	XMMATRIX transposed = XMMatrixTranspose( worldMat * viewMat * projMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&m_constantBuffer.wvpMat, transposed); // store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(constantBufferGPUAddress, &m_constantBuffer, sizeof(m_constantBuffer));

	m_mesh->PopulateCommandList(commandList, constantBufferUploadHeap);

	return true;
}

void Model::Update(float delta)
{
}

void Model::Release()
{
	MeshCache::Release(m_objPath, m_texturePath);
}

void Model::Unload()
{
	MeshCache::Unload(m_objPath, m_texturePath);
	m_mesh = nullptr;
}

bool Model::Intersects(BoundingVolume & cameraBounds, XMFLOAT3 & resolution)
{
	if (m_solid) {
		BoundingVolume* meshBoundingVolume = MeshCache::GetBoundingVolume(m_objPath, m_texturePath);
		meshBoundingVolume->Update(&m_position, &m_rotation);
		if (cameraBounds.IntersectsAlongAxes(meshBoundingVolume, resolution)) {
			if (cameraBounds.IntersectsExactly(meshBoundingVolume, resolution) && meshBoundingVolume->IntersectsExactly(&cameraBounds, resolution)) {
				return true;
			}
		}
	}
	return false;
}

bool Model::CheckCull(BoundingVolume*& frustum)
{

	BoundingVolume* meshBoundingVolume = MeshCache::GetBoundingVolume(m_objPath, m_texturePath);
	meshBoundingVolume->Update(&m_position, &m_rotation);
	if (frustum->CullAlongAxes(meshBoundingVolume)) {
		if (frustum->CullExactly(meshBoundingVolume) && meshBoundingVolume->CullExactly(frustum)) {
			return true;
		}
	}
}


XMFLOAT3 Model::getPosition()
{
	return XMFLOAT3(m_position);
}

void Model::setPosition(XMFLOAT3 pos)
{
	m_position = XMFLOAT3(pos);
}

Mesh * Model::GetMesh()
{
	return m_mesh;
}


void Model::SetCull(bool cullModel, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator)
{
	if (m_isCulled != cullModel)
	{
		if (cullModel)
		{

			if (MeshCache::IsReleased(m_objPath, m_texturePath))
			{
				MeshCache::GetMesh(m_objPath, m_texturePath, m_mesh);
				ComPtr<ID3D12Device> device = MeshCache::GetDevice();
				m_mesh->LoadResources(device, commandList, commandAllocator);
				m_reload = true;
			} else
			{
				MeshCache::GetMesh(m_objPath, m_texturePath, m_mesh);
			}
		}
		else {
			this->Unload();
		}
	}
	m_isCulled = cullModel;
}