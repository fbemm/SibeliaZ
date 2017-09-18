#ifndef _EDGE_STORAGE_H_
#define _EDGE_STORAGE_H_

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>

#include "junctionapi.h"
#include "streamfastaparser.h"

namespace Sibelia
{	
	using std::min;
	using std::max;

	class Edge
	{
	public:
		Edge() : startVertex_(INT64_MAX), endVertex_(INT64_MAX) {}

		Edge(int64_t startVertex, int64_t endVertex, char ch, char revCh, int32_t length, int32_t capacity) :
			startVertex_(startVertex), endVertex_(endVertex), ch_(ch), revCh_(revCh), length_(length), capacity_(capacity)
		{

		}

		int64_t GetFlow() const
		{
			return flow_;
		}

		void AddFlow(int64_t addFlow)
		{
			flow_ += addFlow;
		}

		int64_t GetStartVertex() const
		{
			return startVertex_;
		}

		int64_t GetEndVertex() const
		{
			return endVertex_;
		}

		char GetChar() const
		{
			return ch_;
		}

		int64_t GetLength() const
		{
			return length_;
		}

		int64_t GetCapacity() const
		{
			return capacity_;
		}

		Edge Reverse() const
		{
			return Edge(-endVertex_, -startVertex_, revCh_, ch_, length_, capacity_);
		}

		char GetRevChar() const
		{
			return revCh_;
		}

		bool operator < (const Edge & e) const
		{
			if (startVertex_ != e.startVertex_)
			{
				return startVertex_ < e.startVertex_;
			}

			if (endVertex_ != e.endVertex_)
			{
				return endVertex_ < e.endVertex_;
			}

			if (ch_ != e.ch_)
			{
				return ch_ < e.ch_;
			}

			return false;
		}

		bool Valid() const
		{
			return startVertex_ != INT64_MAX;
		}

		bool operator == (const Edge & e) const
		{
			return startVertex_ == e.startVertex_ && endVertex_ == e.endVertex_ && ch_ == e.ch_;
		}

		bool operator != (const Edge & e) const
		{
			return !(*this == e);
		}
		
		void Inc()
		{
			capacity_++;
		}

	private:
		int64_t startVertex_;
		int64_t endVertex_;
		char ch_;
		char revCh_;
		int32_t flow_;
		int32_t length_;
		int32_t capacity_;		
	};

	class EdgeStorage
	{
	public:

		int64_t GetMaxId() const
		{
			return maxId_;
		}

		int64_t IngoingEdgesNumber(int64_t vertexId) const
		{
			return ingoingEdge_[vertexId + GetMaxId()].size();
		}

		int64_t OutgoingEdgesNumber(int64_t vertexId) const
		{
			return outgoingEdge_[vertexId + GetMaxId()].size();
		}
		
		Edge IngoingEdge(int64_t vertexId, int64_t idx) const
		{
			return edgeList_[ingoingEdge_[vertexId + GetMaxId()][idx]];
		}

		Edge OutgoingEdge(int64_t vertexId, int64_t idx) const
		{
			return edgeList_[outgoingEdge_[vertexId + GetMaxId()][idx]];
		}

		Edge GreedyOutEdge(int64_t vertexId) const
		{
			int64_t vid = vertexId + GetMaxId();
			if (outgoingEdge_[vid].empty())
			{
				return Edge();
			}
						
			size_t idx = *std::max_element(outgoingEdge_[vid].begin(), outgoingEdge_[vid].end(), std::bind(&EdgeStorage::OutEdgeResidualCapacity, this, vertexId, std::placeholders::_1));
			return edgeList_[outgoingEdge_[vid][idx]];
		}

		void Dump(std::ostream & out) const
		{
			out << "digraph G\n{\nrankdir = LR" << std::endl;
			for (const Edge & e : edgeList_)
			{
				out << e.GetStartVertex() << " -> " << e.GetEndVertex() << "[label=\"" << e.GetChar() << ", " << e.GetCapacity() << "\"]" << std::endl;
			}

			out << "}" << std::endl;
		}
		
		void Init(const std::string & inFileName, const std::string & genomesFileName)
		{
			size_t record = 0;
			std::vector<std::string> sequence;
			for (TwoPaCo::StreamFastaParser parser(genomesFileName); parser.ReadRecord(); record++)
			{
				sequence.push_back(std::string());
				for (char ch; parser.GetChar(ch); )
				{
					sequence[record].push_back(ch);
				}
			}

			{
				TwoPaCo::JunctionPositionReader reader(inFileName);
				for (TwoPaCo::JunctionPosition junction; reader.NextJunctionPosition(junction);)
				{
					maxId_ = max(maxId_, junction.GetId());
					validJunctions_.push_back(junction.GetId());
				}

				std::sort(validJunctions_.begin(), validJunctions_.end());
				validJunctions_.erase(std::unique(validJunctions_.begin(), validJunctions_.end()), validJunctions_.end());
			}

			ingoingEdge_.resize(maxId_ * 2 + 1);
			outgoingEdge_.resize(maxId_ * 2 + 1);

			TwoPaCo::JunctionPosition prevJunction;
			TwoPaCo::JunctionPositionReader reader(inFileName);
			for (TwoPaCo::JunctionPosition junction; reader.NextJunctionPosition(junction);)
			{				
				if (junction.GetChr() == prevJunction.GetChr())
				{
					char ch = sequence[prevJunction.GetChr()][prevJunction.GetPos() + k_];
					char revCh = TwoPaCo::DnaChar::ReverseChar(sequence[junction.GetChr()][junction.GetPos() - 1]);
					Edge e(prevJunction.GetId(), junction.GetId(), ch, revCh, junction.GetPos() - prevJunction.GetPos(), 1);
					AddEdge(e);
					AddEdge(e.Reverse());
				}

				prevJunction = junction;
			}			
		}

		EdgeStorage() {}
		EdgeStorage(const std::string & fileName, const std::string & genomesFileName, uint64_t k) : k_(k), maxId_(INT64_MIN)
		{
			Init(fileName, genomesFileName);
		}


	private:
		
		void AddEdge(const Edge & edge)
		{
			int64_t v = edge.GetStartVertex() + maxId_;
			for (auto idx : outgoingEdge_[v])
			{
				if (edgeList_[idx] == edge)
				{
					edgeList_[idx].Inc();
					return;
				}
			}

			ingoingEdge_[edge.GetEndVertex() + maxId_].push_back(edgeList_.size());
			outgoingEdge_[edge.GetStartVertex() + maxId_].push_back(edgeList_.size());
			edgeList_.push_back(edge);
		}

		int64_t OutEdgeResidualCapacity(int64_t v, int64_t idx) const
		{
			const Edge & e = edgeList_[outgoingEdge_[v + maxId_][idx]];
			return e.GetCapacity() - e.GetFlow();
		}

		int64_t k_;
		int64_t maxId_;
		std::vector<Edge> edgeList_;
		std::vector<int64_t> validJunctions_;
		std::vector<std::vector<size_t> > ingoingEdge_;
		std::vector<std::vector<size_t> > outgoingEdge_;
	};
}

#endif