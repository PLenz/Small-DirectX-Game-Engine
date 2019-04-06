#pragma once
#include <map>
#include "BoundingVolume.h"
#include "Mesh.h"

struct MeshCacheObj {
  Mesh* mesh;
  BoundingVolume* bounding_volume;
  int ref_counter = 0;


  bool operator==(const MeshCacheObj& obj) {
    return mesh->Compare(*obj.mesh);
  }
};

class MeshCache {
public:
  static bool GetMesh(std::wstring obj_path, std::wstring png_path, Mesh*& out_mesh);
  static BoundingVolume* GetBoundingVolume(std::wstring obj_path, std::wstring png_path);
  static void SetBoundingVolume(std::wstring obj_path, std::wstring png_path);
  static void Release(std::wstring obj_path, std::wstring png_path);
  static void Unload(std::wstring obj_path, std::wstring png_path);
  static int GetMeshTableSize();
  static void SetDevice(ComPtr<ID3D12Device>& device);
  static boolean IsReleased(std::wstring obj_path, std::wstring png_path);
  static ComPtr<ID3D12Device> GetDevice();

private:
  static std::map<std::wstring, MeshCacheObj*> m_mesh_table_;
  static ID3D12Device* p_device_;
};
