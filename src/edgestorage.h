#ifndef _EDGE_STORAGE_H_
#define _EDGE_STORAGE_H_

#include <string>
#include <vector>
#include <cstdint>

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

		int64_t GetVerticesNumber() const
		{
			return ingoingEdge_.size();
		}

		int64_t IngoingEdgesNumber(int64_t vertexId) const
		{
			return ingoingEdge_[vertexId + GetVerticesNumber()].size();
		}

		int64_t OutgoingEdgesNumber(int64_t vertexId) const
		{
			return outgoingEdge_[vertexId + GetVerticesNumber()].size();
		}
		
		Edge IngoingEdge(int64_t vertexId, int64_t idx) const
		{
			return ingoingEdge_[vertexId + GetVerticesNumber()][idx];
		}

		Edge OutgoingEdge(int64_t vertexId, int64_t idx) const
		{
			return outgoingEdge_[vertexId + GetVerticesNumber()][idx];
		}

		Edge GreedyOutEdge(int64_t vertexId) const
		{
			int64_t vid = vertexId + GetVerticesNumber();
			if (outgoingEdge_[vid].empty())
			{
				return Edge();
			}

			return *std::max_element(outgoingEdge_[vid].begin(), outgoingEdge_[vid].end(), std::bind(&Edge::GetCapacity));
		}
		
		Edge& FindEdge(Edge & key)
		{
			int64_t v = key.GetStartVertex() + maxId_;
			auto it = std::find(outgoingEdge_[v].begin(), outgoingEdge_[v].end(), key);
			if (it != outgoingEdge_[v].end())
			{
				return edgeList_[*it];
			}

			Edge revKey = key.Reverse();
			int64_t v = key.GetStartVertex() + maxId_;
			auto it = std::find(outgoingEdge_[v].begin(), outgoingEdge_[v].end(), key);
			if (it != outgoingEdge_[v].end())
			{
				return edgeList_[*it];
			}

			return Edge();
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
					maxId_ = std::max(maxId_, junction.GetId());
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
					Edge newEdge(prevJunction.GetId(), junction.GetId(), ch, revCh, junction.GetPos() - prevJunction.GetPos(), 1);
					Edge oldEdge = FindEdge(newEdge);
					if (oldEdge.Valid())
					{
						oldEdge.Inc();
					}
					else
					{
						outgoingEdge_[newEdge.GetStartVertex() + maxId_].push_back(edgeList_.size());
						ingoingEdge_[newEdge.GetEndVertex() + maxId_].push_back(edgeList_.size());
						edgeList_.push_back(newEdge);						
					}
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
		
		int64_t k_;
		int64_t maxId_;
		std::vector<Edge> edgeList_;
		std::vector<int64_t> validJunctions_;
		std::vector<std::vector<size_t> > ingoingEdge_;
		std::vector<std::vector<size_t> > outgoingEdge_;
	};
}

#endif