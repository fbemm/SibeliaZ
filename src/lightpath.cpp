#include "lightpath.h"

namespace Sibelia
{
	void FindLightPaths(GraphStorage & storage,
		int64_t minBlockSize,
		int64_t maxBranchSize,
		int64_t maxFlankingSize,
		int64_t lookingDepth,
		int64_t sampleSize,
		int64_t threads,
		std::vector<std::vector<GraphStorage::Arc> > & ret)
	{
		LightPath path(storage, maxBranchSize, minBlockSize, maxFlankingSize);
		for (auto v : storage.validJunctions_)
		{
			path.TryExtend(v);
		}
	}
}