#include "stdafx.h"
#include "Model.h"
#include "Camera.h"


Model::Model(std::wstring obj_path, std::wstring texture_path, XMFLOAT3 position, XMFLOAT4 rotation, bool solid) {
  m_position_ = position;
  m_rotation_ = rotation;
  m_solid_ = solid;
  m_obj_path_ = obj_path;
  m_texture_path_ = texture_path;
  MeshCache::GetMesh(m_obj_path_, m_texture_path_, m_mesh_);
  m_is_culled_ = true;
}


bool Model::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                          ComPtr<ID3D12CommandAllocator>& command_allocator) {
  MeshCache::SetDevice(device);
  m_mesh_->LoadResources(device, commandList, command_allocator);
  MeshCache::SetBoundingVolume(m_obj_path_, m_texture_path_);
  return true;
}


bool Model::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list, UINT8* constant_buffer_gpu_address,
                                D3D12_GPU_VIRTUAL_ADDRESS constant_buffer_upload_heap) {
  // Wird mit jedem Frame aufgerufen.

  // Update constant buffer
  // create the wvp matrix and store in constant buffer
  XMMATRIX viewMat = XMLoadFloat4x4(&Camera::GetViewMatrix()); // load view matrix
  XMMATRIX projMat = XMLoadFloat4x4(&Camera::GetProjectionMatrix()); // load projection matrix
  XMMATRIX rotatMat = XMMatrixRotationQuaternion(XMLoadFloat4(&m_rotation_));

  XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat3(&m_position_));
  XMMATRIX worldMat = rotatMat * translationMat;

  XMMATRIX transposed = XMMatrixTranspose(worldMat * viewMat * projMat); // must transpose wvp matrix for the gpu
  XMStoreFloat4x4(&m_constant_buffer_.wvpMat, transposed); // store transposed wvp matrix in constant buffer

  // copy our ConstantBuffer instance to the mapped constant buffer resource
  memcpy(constant_buffer_gpu_address, &m_constant_buffer_, sizeof(m_constant_buffer_));

  m_mesh_->PopulateCommandList(command_list, constant_buffer_upload_heap);

  return true;
}


void Model::Update(float delta) {}


void Model::Release() {
  MeshCache::Release(m_obj_path_, m_texture_path_);
}


void Model::Unload() {
  MeshCache::Unload(m_obj_path_, m_texture_path_);
  m_mesh_ = nullptr;
}


bool Model::Intersects(BoundingVolume& camera, XMFLOAT3& resolution) {
  if (m_solid_) {
    BoundingVolume* meshBoundingVolume = MeshCache::GetBoundingVolume(m_obj_path_, m_texture_path_);
    meshBoundingVolume->Update(&m_position_, &m_rotation_);
    if (camera.IntersectsAlongAxes(meshBoundingVolume, resolution)) {
      if (camera.IntersectsExactly(meshBoundingVolume, resolution) && meshBoundingVolume->IntersectsExactly(
        &camera, resolution)) {
        return true;
      }
    }
  }
  return false;
}


bool Model::CheckCull(BoundingVolume*& frustum) {

  BoundingVolume* meshBoundingVolume = MeshCache::GetBoundingVolume(m_obj_path_, m_texture_path_);
  meshBoundingVolume->Update(&m_position_, &m_rotation_);
  if (frustum->CullAlongAxes(meshBoundingVolume)) {
    if (frustum->CullExactly(meshBoundingVolume) && meshBoundingVolume->CullExactly(frustum)) {
      return true;
    }
  }
}


XMFLOAT3 Model::getPosition() {
  return XMFLOAT3(m_position_);
}


void Model::setPosition(XMFLOAT3 pos) {
  m_position_ = XMFLOAT3(pos);
}


Mesh* Model::GetMesh() {
  return m_mesh_;
}


void Model::SetCull(bool cull, ComPtr<ID3D12GraphicsCommandList>& command_list,
                    ComPtr<ID3D12CommandAllocator>& command_allocator) {
  if (m_is_culled_ != cull) {
    if (cull) {

      if (MeshCache::IsReleased(m_obj_path_, m_texture_path_)) {
        MeshCache::GetMesh(m_obj_path_, m_texture_path_, m_mesh_);
        ComPtr<ID3D12Device> device = MeshCache::GetDevice();
        m_mesh_->LoadResources(device, command_list, command_allocator);
        m_reload_ = true;
      } else {
        MeshCache::GetMesh(m_obj_path_, m_texture_path_, m_mesh_);
      }
    } else {
      this->Unload();
    }
  }
  m_is_culled_ = cull;
}
