#ifndef _LIGHT_PATH_H_
#define _LIGHT_PATH_H_

#include <queue>
#include <functional>
#include <unordered_map>

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

		const std::vector<Point>& GetBody() const
		{
			return body_;
		}

		int64_t Length() const
		{
			return body_.back().EndDistance();
		}

		int64_t Flow() const
		{
			return 0;
		}

		void TryExtend(int64_t origin)
		{
			Init(origin);
			int64_t bestFlow = 0;
			while (Length() < minBlockSize_ && PushBack());
			
			do 
			{
				bestFlow = std::max(bestFlow, CalculateFlow());
			} while (Length() < minChainSize_ + maxFlankingSize_ && PushBack());

			if (bestFlow > 1)
			{
				while (PushBack() && CalculateFlow() == bestFlow);
			}
			
		}

		void FinalizeFlow(EdgeStorage & storage)
		{

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
	
		int64_t origin_;
		int64_t minChainSize_;
		int64_t minBlockSize_;
		int64_t maxBranchSize_;
		int64_t maxFlankingSize_;
		const EdgeStorage * storage_;
		std::vector<Point> body_;

		void Init(int64_t origin)
		{
			body_.clear();
			origin_ = origin;
		}

		bool PushBack()
		{
			int64_t distance = body_.empty() ? 0 : body_.back().EndDistance();
			int64_t end = body_.empty() ? origin_ : body_.back().Edge().GetEndVertex();
			Edge e = storage_->RandomOutEdge(end);
			if (e.Valid())
			{
				body_.push_back(Point(e, distance));
				return true;
			}

			return false;			
		}

		struct State
		{
			int64_t cost;
			int64_t vertex;
			int64_t prevBodyVertex;
			State(int64_t vertex, int64_t prevBodyVertex, int64_t cost) : vertex(vertex), prevBodyVertex(prevBodyVertex), cost(cost)
			{

			}

			bool operator < (const State & b) const
			{
				return cost < b.cost;
			}
		};

		struct StateHash
		{			
			std::hash<int64_t> f;
			int64_t operator()(const State & state) const
			{
				return f(state.vertex | state.prevBodyVertex << (int64_t(32)));
			}
		};
	
		int64_t CalculateFlow()
		{
			std::priority_queue<State> q;
			std::unordered_map<int64_t, int64_t> currentFlow;
			std::unordered_map<State, int64_t, StateHash> minCost;
		}

	};

	void FindLightPaths(EdgeStorage & storage,		
		int64_t minBlockSize,
		int64_t maxBranchSize,
		int64_t maxFlankingSize,
		int64_t lookingDepth,
		int64_t sampleSize,
		int64_t threads,
		std::vector<std::vector<Edge> > & ret);
}

#endif