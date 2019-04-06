#pragma once

class Projection
{
public:
	Projection(XMFLOAT3 min, XMFLOAT3 max);

	bool overlap(Projection other);
	XMFLOAT3 getOverlap(Projection other);

private:
	XMFLOAT3 m_min;
	XMFLOAT3 m_max;
};

