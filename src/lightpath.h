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

		void FinalizeFlow(EdgeStorage & storage) const
		{

		}

	private:
	
		struct State
		{
			int64_t cost;
			int64_t vertex;
			int64_t prevBodyVertex;
			Edge e;
			State(int64_t vertex, int64_t prevBodyVertex, Edge e, int64_t cost) : vertex(vertex), e(e), prevBodyVertex(prevBodyVertex), cost(cost)
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

		int64_t origin_;
		int64_t minChainSize_;
		int64_t minBlockSize_;
		int64_t maxBranchSize_;
		int64_t maxFlankingSize_;
		std::vector<Point> body_;
		const EdgeStorage * storage_;
		std::unordered_map<int64_t, int64_t> distance_;
		std::unordered_map<State, int64_t, StateHash> minCost_;
		std::unordered_map<Edge, int64_t, EdgeHash> currentFlow_;		

		void Init(int64_t origin)
		{
			body_.clear();
			origin_ = origin;
			distance_[origin_] = 0;
		}

		bool PushBack()
		{
			int64_t distance = body_.empty() ? 0 : body_.back().EndDistance();
			int64_t end = body_.empty() ? origin_ : body_.back().Edge().GetEndVertex();
			Edge e = storage_->RandomOutEdge(end);
			if (e.Valid())
			{
				body_.push_back(Point(e, distance));
				distance_[e.GetEndVertex()] = body_.back().EndDistance();
				return true;
			}

			return false;			
		}		
	
		int64_t CalculateFlow()
		{
			minCost_.clear();
			currentFlow_.clear();
			std::priority_queue<State> q;
			q.push(State(origin_, origin_, Edge(), 0));
			while (!q.empty())
			{
				State now = q.top();
				q.pop();
				if (distance_[now.vertex] < body_.back().EndDistance() - maxBranchSize_)
				{

				}
				else
				{

				}
			}
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