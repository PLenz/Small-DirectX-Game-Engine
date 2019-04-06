#include "stdafx.h"
#include "MeshCache.h"

std::map<std::wstring, MeshCacheObj*> MeshCache::m_mesh_table_;
ID3D12Device* MeshCache::p_device_ = nullptr;


bool MeshCache::GetMesh(std::wstring obj_path, std::wstring png_path, Mesh*& out_mesh) {
  const std::wstring full = obj_path + png_path;
  if (m_mesh_table_.find(full) == m_mesh_table_.end()) {
    Mesh* mesh = new Mesh(obj_path, png_path);

    MeshCacheObj* obj = new MeshCacheObj();
    obj->mesh = mesh;
    obj->ref_counter++;

    m_mesh_table_[full] = obj;
    out_mesh = m_mesh_table_[full]->mesh;
  } else {
    if (m_mesh_table_[full]->mesh == nullptr) {
      Mesh* mesh = new Mesh(obj_path, png_path);
      m_mesh_table_[full]->mesh = mesh;
    }
    m_mesh_table_[full]->ref_counter++;
    out_mesh = m_mesh_table_[full]->mesh;
  }

  return true;
}


void MeshCache::Release(std::wstring obj_path, std::wstring png_path) {
  m_mesh_table_.clear();
}


void MeshCache::Unload(std::wstring obj_path, std::wstring png_path) {
  const std::wstring full = obj_path + png_path;
  if (!(m_mesh_table_.find(full) == m_mesh_table_.end())) {
    m_mesh_table_[full]->ref_counter--;
    if (m_mesh_table_[full]->ref_counter == 0) {
      m_mesh_table_[full]->mesh->Release();
      m_mesh_table_[full]->mesh = nullptr;
    }
  }
}


void MeshCache::SetBoundingVolume(std::wstring obj_path, std::wstring png_path) {
  const std::wstring full = obj_path + png_path;
  if (!(m_mesh_table_.find(full) == m_mesh_table_.end())) {
    if (m_mesh_table_[full]->bounding_volume == nullptr) {
      m_mesh_table_[full]->bounding_volume = new BoundingVolume(*(m_mesh_table_[full]->mesh->GetBoundingVolume()));
    }
  }
}


BoundingVolume* MeshCache::GetBoundingVolume(std::wstring obj_path, std::wstring png_path) {
  const std::wstring full = obj_path + png_path;
  if (!(m_mesh_table_.find(full) == m_mesh_table_.end())) {
    return m_mesh_table_[full]->bounding_volume;
  }

  return nullptr;
}


int MeshCache::GetMeshTableSize() {
  return m_mesh_table_.size();
}


void MeshCache::SetDevice(ComPtr<ID3D12Device>& device) {
  if (p_device_ == nullptr) {
    p_device_ = device.Get();
  }
}


boolean MeshCache::IsReleased(std::wstring obj_path, std::wstring png_path) {
  const std::wstring full = obj_path + png_path;
  if (!(m_mesh_table_.find(full) == m_mesh_table_.end())) {
    return m_mesh_table_[full]->ref_counter == 0;
  }
  return true;
}


ComPtr<ID3D12Device> MeshCache::GetDevice() {
  return ComPtr<ID3D12Device>(p_device_);
}
