#include "stdafx.h"
#include "Projection.h"
#include <limits>

Projection::Projection(XMFLOAT3 min, XMFLOAT3 max)
{
	this->m_min = min;
	this->m_max = max;
}

bool Projection::overlap(Projection other)
{
	return m_min.x < other.m_max.x && m_max.x > other.m_min.x &&
		m_min.z < other.m_max.z && m_max.z > other.m_min.z;
}

XMFLOAT3 Projection::getOverlap(Projection other)
{
	XMFLOAT3 resolution;
	float xleft = m_max.x - other.m_min.x;
	float xright = m_min.x - other.m_max.x;
	resolution.x = abs(xleft) < abs(xright) ? xleft : xright;

	float zleft = m_max.z - other.m_min.z;
	float zright = m_min.z - other.m_max.z;
	resolution.z = abs(zleft) < abs(zright) ? zleft : zright;

	// TODO: Für eine achse entscheiden
	if (abs(resolution.x) < abs(resolution.z)) {
		resolution.z = 0.0f;
	}
	else {
		resolution.x = 0.0f;
	}

	return resolution;
}
