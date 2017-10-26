#ifndef _LIGHT_PATH_H_
#define _LIGHT_PATH_H_

#include <queue>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "graphstorage.h"

namespace Sibelia
{
	class LightPath
	{
	public:
		LightPath(GraphStorage & storage,
			int64_t maxBranchSize,
			int64_t minBlockSize,
			int64_t maxFlankingSize) :
			maxBranchSize_(maxBranchSize), minBlockSize_(minBlockSize), maxFlankingSize_(maxFlankingSize), storage_(storage),
			minChainSize_(minBlockSize - 2 * maxFlankingSize)
		{

		}

		struct Point
		{
		private:
			GraphStorage::Arc arc;
			int64_t startDistance;
		public:
			Point() {}
			Point(GraphStorage::Arc arc, int64_t startDistance) : arc(arc), startDistance(startDistance) {}

			GraphStorage::Node Vertex(GraphStorage::Graph & g) const
			{
				return g.target(arc);
			}

			GraphStorage::Arc Arc() const
			{
				return arc;
			}

			int64_t StartDistance() const
			{
				return startDistance;
			}
			
			bool operator == (const Point & p) const
			{
				return startDistance == p.startDistance && arc == p.arc;
			}

			bool operator != (const Point & p) const
			{
				return p != *this;
			}
		};		

		const std::vector<Point>& GetBody() const
		{
			return body_;
		}

		int64_t Length() const
		{
			return body_.back().StartDistance();
		}

		int64_t Flow() const
		{
			return 0;
		}

		void TryExtend(int64_t origin)
		{
			body_.clear();
		}

		void FinalizeFlow(GraphStorage & storage) const
		{

		}

	private:

		int64_t origin_;
		int64_t minChainSize_;
		int64_t minBlockSize_;
		int64_t maxBranchSize_;
		int64_t maxFlankingSize_;
		std::vector<Point> body_;
		std::unordered_set<int64_t> insideBody_;
		GraphStorage & storage_;
			
		bool PushBack()
		{
			int64_t prevDistance = body_.empty() ? 0 : body_.back().StartDistance();
//			int64_t prevVertex = body_.empty() ? origin_ : body_.back().Edge().GetEndVertex();
			/*
			Edge e = storage_->RandomOutEdge(prevVertex);
			if (e.Valid() && e.GetEndVertex() != origin_ && insideBody_.find(e.GetEndVertex()) == insideBody_.end())
			{
				body_.push_back(Point(e, prevDistance + e.GetLength()));
				insideBody_.insert(e.GetEndVertex());
				return true;
			}
			*/
			return false;			
		}		
	
		int64_t CalculateFlow()
		{

		}

	};

	void FindLightPaths(GraphStorage & storage,		
		int64_t minBlockSize,
		int64_t maxBranchSize,
		int64_t maxFlankingSize,
		int64_t lookingDepth,
		int64_t sampleSize,
		int64_t threads,
		std::vector<std::vector<GraphStorage::Arc> > & ret);
}

#endif