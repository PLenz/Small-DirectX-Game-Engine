#pragma once
#include <map>
#include "BoundingVolume.h"
#include "Mesh.h"

struct MeshCacheObj {
	Mesh* mesh;
	BoundingVolume* boundingVolume;
	int refCounter = 0;
	bool operator==(const MeshCacheObj& r)
	{
		return mesh->Compare(*r.mesh);
	}
};

class MeshCache
{
public:
	static bool GetMesh(std::wstring objPath, std::wstring pngPath, Mesh*& outMesh);
	static BoundingVolume* GetBoundingVolume(std::wstring objPath, std::wstring pngPath);
	static void SetBoundingVolume(std::wstring objPath, std::wstring pngPath);
	static void Release(std::wstring objPath, std::wstring pngPath);
	static void Unload(std::wstring objPath, std::wstring pngPath);
	static int GetMeshTableSize();
	static void SetDevice(ComPtr<ID3D12Device>& device);
	static boolean IsReleased(std::wstring objPath, std::wstring pngPath);
	static ComPtr<ID3D12Device> GetDevice();

private:
	static std::map<std::wstring, MeshCacheObj*> m_meshTable;
	static ID3D12Device* p_device;
};