#include "lightpath.h"

namespace Sibelia
{
	void FindLightPaths(EdgeStorage & storage,
		int64_t minBlockSize,
		int64_t maxBranchSize,
		int64_t maxFlankingSize,
		int64_t lookingDepth,
		int64_t sampleSize,
		int64_t threads,
		std::vector<std::vector<Edge> > & ret)
	{

	}

	void FindLightPaths(EdgeStorage & storage,
		int64_t minBlockSize,
		int64_t maxBranchSize,
		int64_t maxFlankingSize,
		int64_t lookingDepth,
		int64_t sampleSize,
		int64_t threads,
		std::vector<std::vector<Edge> > & ret)
	{
		LightPath path(storage, minBlockSize, maxBranchSize, maxFlankingSize);
		for (int64_t v : storage.ValidJunctions())
		{
			for(bool success = true; success;)
			{
				success = false;
				for (int64_t sample = 0; sample < sampleSize && !success; sample++)
				{
					path.TryExtend(v);
					if (path.Flow() > 1 && path.Length() >= minBlockSize)
					{
						success = true;
						path.FinalizeFlow(storage);
					}					
				}				
			}			
		}
	}
}