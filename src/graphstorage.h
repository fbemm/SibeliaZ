#ifndef _GRAPH_STORAGE_H_
#define _GRAPH_STORAGE_H_

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>

#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>

#include "junctionapi.h"
#include "streamfastaparser.h"

namespace Sibelia
{
	class GraphStorage
	{
	public:

		typedef lemon::ListDigraph Graph;
		typedef lemon::ListDigraph::Arc Arc;
		typedef lemon::ListDigraph::Node Node;		

		GraphStorage(const std::string & junctionsFileName, const std::string & genomesFileName, uint64_t k) : k_(k), maxId_(INT64_MIN),
			arcChar_(g_), length_(g_), capacityLowerBound_(g_), capacityUpperBound_(g_)
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
				TwoPaCo::JunctionPositionReader reader(junctionsFileName);
				for (TwoPaCo::JunctionPosition junction; reader.NextJunctionPosition(junction);)
				{
					maxId_ = std::max(maxId_, junction.GetId());
				}
			}

			inNode_.resize(maxId_ * 2 + 1, lemon::INVALID);
			outNode_.resize(maxId_ * 2 + 1, lemon::INVALID);
			std::vector<bool> seen(maxId_ * 2 + 1, false);

			TwoPaCo::JunctionPosition prevJunction;
			TwoPaCo::JunctionPositionReader reader(junctionsFileName);
			for (TwoPaCo::JunctionPosition junction; reader.NextJunctionPosition(junction);)
			{
				if (!seen[Vid(junction.GetId())])
				{
					seen[Vid(junction.GetId())] = true;
					validJunctions_.push_back(junction.GetId());
				}

				if (junction.GetChr() == prevJunction.GetChr())
				{
					char ch = sequence[prevJunction.GetChr()][prevJunction.GetPos() + k_];
					char revCh = TwoPaCo::DnaChar::ReverseChar(sequence[junction.GetChr()][junction.GetPos() - 1]);
					AddEdge(prevJunction.GetId(), junction.GetId(), junction.GetPos() - prevJunction.GetPos(), ch);
					AddEdge(-junction.GetId(), -prevJunction.GetId(), junction.GetPos() - prevJunction.GetPos(), revCh);
				}

				prevJunction = junction;
			}
		}

		lemon::ListDigraph::Arc FindArc(lemon::ListDigraph::Node u, char ch)
		{
			for (lemon::ListDigraph::OutArcIt arcIt(g_, u); arcIt != lemon::INVALID; ++arcIt)
			{
				if (arcChar_[arcIt] == ch)
				{
					return arcIt;
				}
			}

			return lemon::INVALID;
		}

		void DumpLight(std::ostream & out) const
		{
			out << "digraph G\n{\nrankdir = LR" << std::endl;
			for (lemon::ListDigraph::ArcIt arcIt(g_); arcIt != lemon::INVALID; ++arcIt)
			{
				lemon::ListDigraph::Arc a = arcIt;
				char ch = arcChar_[arcIt] != 0 ? arcChar_[arcIt] : 'D';
				int cap = capacityUpperBound_[arcIt] != INT32_MAX ? capacityUpperBound_[arcIt] : -1;
				out << g_.id(g_.source(arcIt)) << " -> " << g_.id(g_.target(arcIt)) << "[label=\"" << ch << ", " << cap << "\"]" << std::endl;				
			}
			
			out << "}" << std::endl;
		}

		lemon::ListDigraph g_;
		std::vector<int64_t> validJunctions_;
		std::vector<lemon::ListDigraph::Node> inNode_;
		std::vector<lemon::ListDigraph::Node> outNode_;
		lemon::ListDigraph::ArcMap<int> length_;
		lemon::ListDigraph::ArcMap<char> arcChar_;
		lemon::ListDigraph::ArcMap<int> capacityLowerBound_;
		lemon::ListDigraph::ArcMap<int> capacityUpperBound_;

	private:

		int64_t Vid(int64_t v) const
		{
			return v + maxId_;
		}

		void AddNode(int64_t id)
		{
			int64_t vid = Vid(id);
			if (!g_.valid(inNode_[Vid(id)]))
			{
				inNode_[vid] = g_.addNode();
				outNode_[vid] = g_.addNode();
				auto e = g_.addArc(inNode_[vid], outNode_[vid]);
				capacityUpperBound_[e] = INT32_MAX;
			}
		}
		
		void AddEdge(int64_t start, int64_t end, int64_t length, char ch)
		{
			int64_t startId = Vid(start);
			int64_t endId = Vid(end);
			AddNode(start);
			AddNode(end);
			lemon::ListDigraph::Arc arc = FindArc(outNode_[startId], ch);
			if (g_.valid(arc))
			{
				capacityUpperBound_[arc] = capacityUpperBound_[arc] + 1;
			}
			else
			{
				arc = g_.addArc(outNode_[startId], inNode_[endId]);
				arcChar_[arc] = ch;
				capacityUpperBound_[arc] = 1;
			}

		}

		int64_t k_;
		int64_t maxId_;		
	};

}

#endif