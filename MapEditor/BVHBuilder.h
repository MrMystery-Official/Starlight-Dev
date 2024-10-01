#pragma once

#include "bvh/bvh.hpp"

using Scalar = float;
using Bvh = bvh::Bvh<Scalar>;

namespace BVHBuilder
{
	extern Bvh mBVH;

	bool BuildBVHForDomains(const float* domains, int domainCount);
	bool BuildBVHForMesh(const float* verts, const unsigned int* indices, int icount);
	size_t GetNodeSize();
	size_t GetBVHSize();
	std::vector<Bvh::Node> GetBVHNodes();
};