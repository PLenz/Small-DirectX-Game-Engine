#include "stdafx.h"
#include "MeshCache.h"

std::map<std::wstring, MeshCacheObj*> MeshCache::m_meshTable;
ID3D12Device* MeshCache::p_device = nullptr;

bool MeshCache::GetMesh(std::wstring objPath, std::wstring pngPath, Mesh*& outMesh)
{
	const std::wstring full = objPath + pngPath;
	if (m_meshTable.find(full) == m_meshTable.end())
	{
		Mesh* mesh = new Mesh(objPath, pngPath);
		
		MeshCacheObj* obj = new MeshCacheObj();
		obj->mesh = mesh;
		obj->refCounter++;

		m_meshTable[full] = obj;
		outMesh = m_meshTable[full]->mesh;
	}
	else
	{
		if (m_meshTable[full]->mesh == nullptr)
		{
			Mesh* mesh = new Mesh(objPath, pngPath);
			m_meshTable[full]->mesh = mesh;
		}
		m_meshTable[full]->refCounter++;
		outMesh = m_meshTable[full]->mesh;
	}

	return true;
}

void MeshCache::Release(std::wstring objPath, std::wstring pngPath)
{
	m_meshTable.clear();
}

void MeshCache::Unload(std::wstring objPath, std::wstring pngPath)
{
	const std::wstring full = objPath + pngPath;
	if (!(m_meshTable.find(full) == m_meshTable.end()))
	{
		m_meshTable[full]->refCounter--;
		if (m_meshTable[full]->refCounter == 0) {
			m_meshTable[full]->mesh->Release();
			m_meshTable[full]->mesh = nullptr;
		}
	}
}

void MeshCache::SetBoundingVolume(std::wstring objPath, std::wstring pngPath)
{
	const std::wstring full = objPath + pngPath;
	if (!(m_meshTable.find(full) == m_meshTable.end()))
	{
		if (m_meshTable[full]->boundingVolume == nullptr)
		{
			m_meshTable[full]->boundingVolume = new BoundingVolume(*(m_meshTable[full]->mesh->GetBoundingVolume()));
		}
	}
}

BoundingVolume* MeshCache::GetBoundingVolume(std::wstring objPath, std::wstring pngPath)
{
	const std::wstring full = objPath + pngPath;
	if (!(m_meshTable.find(full) == m_meshTable.end()))
	{
		return m_meshTable[full]->boundingVolume;
	}

	return nullptr;
}

int MeshCache::GetMeshTableSize()
{
	return m_meshTable.size();
}

void MeshCache::SetDevice(ComPtr<ID3D12Device>& device)
{
	if (p_device == nullptr)
	{
		p_device = device.Get();
	}
}

boolean MeshCache::IsReleased(std::wstring objPath, std::wstring pngPath)
{
	const std::wstring full = objPath + pngPath;
	if (!(m_meshTable.find(full) == m_meshTable.end()))
	{
		return m_meshTable[full]->refCounter == 0;
	}
	return true;
}

ComPtr<ID3D12Device> MeshCache::GetDevice()
{
	return ComPtr<ID3D12Device>(p_device);
}
