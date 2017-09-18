#ifndef _LIGHT_PATH_H_
#define _LIGHT_PATH_H_

#include <functional>
#include "edgestorage.h"

namespace Sibelia
{
	class LightPath
	{
	public:
		LightPath(const EdgeStorage & storage,
			int64_t maxBranchSize,
			int64_t minBlockSize,
			int64_t maxFlankingSize) :
			maxBranchSize_(maxBranchSize), minBlockSize_(minBlockSize), maxFlankingSize_(maxFlankingSize), storage_(&storage),
			minChainSize_(minBlockSize - 2 * maxFlankingSize)
		{

		}

		void Init(Edge origin)
		{
			body_.clear();			
			body_.push_back(Point(origin, 0));
		}
		/*
		bool TakeStep()
		{
			while (body_.back().EndDistance() < minBlockSize_)
			{
				Edge e = storage_->GreedyOutEdge(body_.back().Edge().GetEndVertex());
				if (!e.Valid())
				{
					
				}
			}

			
			while(body_.)
			if (e.Valid())
			{
				body_.push_back(Point(e, body_.back().StartDistance() + body_.back().Edge().GetLength()));

			}
		}
		*/
	

	private:

		struct Point
		{
		private:
			Edge edge;
			int64_t startDistance;
		public:
			Point() {}
			Point(Edge edge, int64_t startDistance) : edge(edge), startDistance(startDistance) {}

			Edge Edge() const
			{
				return edge;
			}

			int64_t StartDistance() const
			{
				return startDistance;
			}

			int64_t EndDistance() const
			{
				return startDistance + edge.GetLength();
			}

			bool operator == (const Point & p) const
			{
				return startDistance == p.startDistance && edge == p.edge;
			}

			bool operator != (const Point & p) const
			{
				return p != *this;
			}
		};

		Edge origin_;
		int64_t minChainSize_;
		int64_t minBlockSize_;
		int64_t maxBranchSize_;
		int64_t maxFlankingSize_;
		const EdgeStorage * storage_;
		std::vector<Point> body_;

		int64_t CalculateFlow()
		{

		}

	};
}

#endif