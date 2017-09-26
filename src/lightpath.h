#ifndef _LIGHT_PATH_H_
#define _LIGHT_PATH_H_

#include <queue>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <lemon/smart_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/list_graph.h>
#include <lemon/cycle_canceling.h>

#include <lemon/preflow.h>

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
			Edge edge;
			int64_t startDistance;
		public:
			Point() {}
			Point(Edge edge, int64_t startDistance) : edge(edge), startDistance(startDistance) {}

			int64_t Vertex() const
			{
				return edge.GetEndVertex();
			}

			Edge Edge() const
			{
				return edge;
			}

			int64_t StartDistance() const
			{
				return startDistance;
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
			return body_.back().StartDistance();
		}

		int64_t Flow() const
		{
			return 0;
		}

		void TryExtend(int64_t origin)
		{
			Init(origin);			
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
		

		void Init(int64_t origin)
		{
			body_.clear();
		}

		bool PushBack()
		{
			int64_t prevDistance = body_.empty() ? 0 : body_.back().StartDistance();
			int64_t prevVertex = body_.empty() ? origin_ : body_.back().Edge().GetEndVertex();
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
		std::vector<std::vector<Edge> > & ret);
}

#endif