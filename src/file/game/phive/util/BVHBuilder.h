#pragma once

#include <bvh/bvh.hpp>
#include <vector>

using Scalar = float;
using Bvh = bvh::Bvh<Scalar>;

namespace application::file::game::phive::util
{
	namespace BVHBuilder
	{
		extern Bvh gBVH;

		bool BuildBVHForDomains(const float* domains, int domainCount);
		bool BuildBVHForQuadMesh(const float* verts, const unsigned int* indices, int icount);
		bool BuildBVHForMesh(const float* verts, const unsigned int* indices, int icount);
		size_t GetNodeSize();
		size_t GetBVHSize();
		std::vector<Bvh::Node> GetBVHNodes();
	};
}