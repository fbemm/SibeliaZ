#ifndef _TRASERVAL_H_
#define _TRAVERSAL_H_

//#define _DEBUG_OUT

#include <set>
#include <map>
#include <list>
#include <ctime>
#include <iterator>
#include <cassert>
#include <numeric>
#include <sstream>
#include <iostream>

#include "junctionstorage.h"

namespace Sibelia
{
	extern const std::string DELIMITER;

	class BlockInstance
	{
	public:
		BlockInstance() {}
		BlockInstance(int id, const size_t chr, size_t start, size_t end) : id_(id), chr_(chr), start_(start), end_(end) {}
		void Reverse();
		int GetSignedBlockId() const;
		bool GetDirection() const;
		int GetBlockId() const;		
		int GetSign() const;
		size_t GetChrId() const;
		size_t GetStart() const;
		size_t GetEnd() const;
		size_t GetLength() const;
		size_t GetConventionalStart() const;
		size_t GetConventionalEnd() const;
		std::pair<size_t, size_t> CalculateOverlap(const BlockInstance & instance) const;
		bool operator < (const BlockInstance & toCompare) const;
		bool operator == (const BlockInstance & toCompare) const;
		bool operator != (const BlockInstance & toCompare) const;
	private:
		int id_;
		size_t start_;
		size_t end_;
		size_t chr_;
	};

	namespace
	{
		const bool COVERED = true;
		typedef std::vector<BlockInstance> BlockList;
		typedef std::pair<size_t, std::vector<BlockInstance> > GroupedBlock;
		typedef std::vector<GroupedBlock> GroupedBlockList;
		bool ByFirstElement(const GroupedBlock & a, const GroupedBlock & b)
		{
			return a.first < b.first;
		}

		std::string IntToStr(size_t x)
		{
			std::stringstream ss;
			ss << x;
			return ss.str();
		}

		template<class Iterator1, class Iterator2>
		void CopyN(Iterator1 it, size_t count, Iterator2 out)
		{
			for (size_t i = 0; i < count; i++)
			{
				*out++ = *it++;
			}
		}

		template<class Iterator>
		Iterator AdvanceForward(Iterator it, size_t step)
		{
			std::advance(it, step);
			return it;
		}

		template<class Iterator>
		Iterator AdvanceBackward(Iterator it, size_t step)
		{
			for (size_t i = 0; i < step; i++)
			{
				--it;
			}

			return it;
		}

		typedef std::pair<size_t, size_t> IndexPair;
		template<class T, class F, class It>
		void GroupBy(std::vector<T> & store, F pred, It out)
		{
			sort(store.begin(), store.end(), pred);
			for (size_t now = 0; now < store.size();)
			{
				size_t prev = now;
				for (; now < store.size() && !pred(store[prev], store[now]); now++);
				*out++ = std::make_pair(prev, now);
			}
		}

		template<class F>
		bool CompareBlocks(const BlockInstance & a, const BlockInstance & b, F f)
		{
			return (a.*f)() < (b.*f)();
		}

		template<class F>
		bool EqualBlocks(const BlockInstance & a, const BlockInstance & b, F f)
		{
			return f(a) == f(b);
		}

		template<class Iterator, class F, class ReturnType>
		struct FancyIterator : public std::iterator<std::forward_iterator_tag, ReturnType>
		{
		public:
			FancyIterator& operator++()
			{
				++it;
				return *this;
			}

			FancyIterator operator++(int)
			{
				FancyIterator ret(*this);
				++(*this);
				return ret;
			}

			bool operator == (FancyIterator toCompare) const
			{
				return it == toCompare.it;
			}

			bool operator != (FancyIterator toCompare) const
			{
				return !(*this == toCompare);
			}

			ReturnType operator * ()
			{
				return f(*it);
			}

			FancyIterator() {}
			FancyIterator(Iterator it, F f) : it(it), f(f) {}

		private:
			F f;
			Iterator it;
		};

		template<class Iterator, class F, class ReturnType>
		FancyIterator<Iterator, F, ReturnType> CFancyIterator(Iterator it, F f, ReturnType)
		{
			return FancyIterator<Iterator, F, ReturnType>(it, f);
		}

	}

	bool compareById(const BlockInstance & a, const BlockInstance & b);
	bool compareByChrId(const BlockInstance & a, const BlockInstance & b);
	bool compareByStart(const BlockInstance & a, const BlockInstance & b);
	
	void CreateOutDirectory(const std::string & path);
	
	class BlocksFinder
	{
	public:

		BlocksFinder(JunctionStorage & storage, size_t k) : storage_(storage), k_(k)
		{
	
		}

		void Dump(std::ostream & out) const;

		void FindBlocks(int64_t minBlockSize, int64_t maxBranchSize, const std::string & debugOut)
		{			
			debugOut_ = debugOut;
			minBlockSize_ = minBlockSize;
			maxBranchSize_ = maxBranchSize;
			blockId_.resize(storage_.GetChrNumber());
			for (size_t i = 0; i < storage_.GetChrNumber(); i++)
			{
				blockId_[i].resize(storage_.GetChrVerticesCount(i));
			}

			int64_t count = 0;
			std::vector<int64_t> shuffle;
			std::ofstream debugStream(debugOut.c_str());
			for (int64_t v = -storage_.GetVerticesNumber() + 1; v < storage_.GetVerticesNumber(); v++)
			{
				for (size_t i = 0; i < storage_.GetInstancesCount(v); i++)
				{
					if (storage_.GetJunctionInstance(v, i).IsPositiveStrand())
					{
						shuffle.push_back(v);
						break;
					}
				}				
			}

			time_t mark = time(0);
			BubbledBranches forwardBubble;
			BubbledBranches backwardBubble;
			std::vector<JunctionStorage::JunctionIterator> instance;			
			for (int64_t vertex : shuffle)
			{
				instance.resize(storage_.GetInstancesCount(vertex));
				for (size_t i = 0; i < storage_.GetInstancesCount(vertex); i++)
				{
					instance[i] = storage_.GetJunctionInstance(vertex, i);
				}

				BubbledBranchesForward(vertex, instance, forwardBubble);
				BubbledBranchesBackward(vertex, instance, backwardBubble);

				for (size_t i = 0; i < forwardBubble.size(); i++)
				{
					for (size_t j = 0; j < forwardBubble[i].size(); j++)
					{
						size_t k = forwardBubble[i][j];
						if (std::find(backwardBubble[i].begin(), backwardBubble[i].end(), k) == backwardBubble[i].end())
						{
							source_.push_back(Fork(instance[i], instance[k]));
						}
					}
				}				
			}

			std::cout << "Done. " << time(0) - mark << std::endl;
			mark = time(0);

			int64_t total = 0;
			for(size_t i = 0; i < source_.size(); i++)
			{	
				Fork sink = ExpandSourceFork(source_[i]);
				total += abs(source_[i].branch[0].GetIndex() - sink.branch[0].GetIndex()) + abs(source_[i].branch[1].GetIndex() - sink.branch[1].GetIndex());
				continue;
				if (ChainLength(source_[i], sink) > minBlockSize)
				{
					std::stringstream fname;
					fname << debugOut << i << ".dot";
					std::ofstream pathOut(fname.str());
					std::vector<std::pair<JunctionStorage::JunctionIterator, JunctionStorage::JunctionIterator> > visit;
					pathOut << "digraph G\n{\nrankdir = LR\n";
					PlotPath(source_[i], sink, pathOut, visit);
					for (int j = 0; j < 2; j++)
					{
						for (auto it = source_[i].branch[j]; it != sink.branch[j]; ++it)
						{
							DumpVertex(it.GetVertexId(&storage_), pathOut, visit, 10);
						}
					}

					pathOut << "}";
				}				
			}

			std::cout << "Done. " << time(0) - mark << std::endl;
			std::cout << total << std::endl;
		}
		


		void GenerateLegacyOutput(const std::string & outDir, const std::string & oldCoords) const;
	
	private:

		template<class Iterator>
		void OutputLines(Iterator start, size_t length, std::ostream & out) const
		{
			for (size_t i = 1; i <= length; i++, ++start)
			{
				out << *start;
				if (i % 80 == 0 && i != length)
				{
					out << std::endl;
				}
			}
		}

		void ListChrs(std::ostream & out) const;
		std::string OutputIndex(const BlockInstance & block) const;
		void TryOpenFile(const std::string & fileName, std::ofstream & stream) const;
		void GenerateReport(const BlockList & block, const std::string & fileName) const;		
		void OutputBlocks(const std::vector<BlockInstance>& block, std::ofstream& out) const;
		void ListBlocksIndices(const BlockList & block, const std::string & fileName) const;		
		void ListBlocksSequences(const BlockList & block, const std::string & fileName) const;
		void ListChromosomesAsPermutations(const BlockList & block, const std::string & fileName) const;
		std::vector<double> CalculateCoverage(GroupedBlockList::const_iterator start, GroupedBlockList::const_iterator end) const;			
				

		template<class T>
		void DumpVertex(int64_t id, std::ostream & out, T & visit, int64_t cnt = 5) const
		{		
			int64_t jcnt = storage_.GetInstancesCount(id);
			for (int64_t idx = 0; idx < jcnt; idx++)
			{
				auto jt = storage_.GetJunctionInstance(id, idx);
				for (int64_t i = 0; i < cnt; i++)
				{
					auto it = jt - 1;
					auto pr = std::make_pair(it, jt);
					if (it.Valid(&storage_) && std::find(visit.begin(), visit.end(), pr) == visit.end())
					{
						out << it.GetVertexId(&storage_) << " -> " << jt.GetVertexId(&storage_)
							<< "[label=\"" << it.GetChar(&storage_) << ", " << it.GetChrId() << ", " << it.GetPosition(&storage_) << ", " << abs(it.GetPosition(&storage_) - jt.GetPosition(&storage_)) << "\""
							<< (it.IsPositiveStrand() ? "color=blue" : "color=red") << "]\n";
						visit.push_back(pr);
					}

					jt = it;
				}
			}

			for (int64_t idx = 0; idx < storage_.GetInstancesCount(id); idx++)
			{
				auto it = storage_.GetJunctionInstance(id, idx);
				for (int64_t i = 0; i < cnt; i++)
				{
					auto jt = it + 1;
					auto pr = std::make_pair(it, jt);
					if (jt.Valid(&storage_) && std::find(visit.begin(), visit.end(), pr) == visit.end())
					{
						out << it.GetVertexId(&storage_) << " -> " << jt.GetVertexId(&storage_)
							<< "[label=\"" << it.GetChar(&storage_) << ", " << it.GetChrId() << ", " << it.GetPosition(&storage_) << ", " << abs(it.GetPosition(&storage_) - jt.GetPosition(&storage_)) << "\""
							<< (it.IsPositiveStrand() ? "color=blue" : "color=red") << "]\n";
						visit.push_back(pr);
					}

					it = jt;
				}
			}
		}

		struct Assignment
		{
			static const int32_t UNKNOWN_BLOCK;
			int32_t block;
			int32_t instance;
			Assignment() : block(UNKNOWN_BLOCK), instance(UNKNOWN_BLOCK)
			{

			}

			bool operator == (const Assignment & assignment) const
			{
				return block == assignment.block && instance == assignment.instance;
			}
		};

		struct BranchData
		{
			std::vector<size_t> branchId;
		};

		typedef std::vector<std::vector<size_t> > BubbledBranches;
		
		struct Fork
		{
			Fork(JunctionStorage::JunctionIterator it, JunctionStorage::JunctionIterator jt)
			{
				branch[0] = std::min(it, jt);
				branch[1] = std::max(it, jt);
			}

			JunctionStorage::JunctionIterator branch[2];
		};

		int64_t ChainLength(const Fork & now, const Fork & next) const
		{
			return std::min(abs(now.branch[0].GetPosition(&storage_) - next.branch[0].GetPosition(&storage_)),
				abs(now.branch[1].GetPosition(&storage_) - next.branch[1].GetPosition(&storage_)));
		}

		Fork ExpandSourceFork(const Fork & source) const
		{
			for (auto now = source; ; )
			{				
				auto next = TakeBubbleStep(now);
				if (next.branch[0].Valid(&storage_))
				{					
					int64_t vid0 = now.branch[0].GetVertexId(&storage_);
					int64_t vid1 = now.branch[1].GetVertexId(&storage_);
					assert(vid0 == vid1 &&
						abs(now.branch[0].GetPosition(&storage_) - next.branch[0].GetPosition(&storage_)) < maxBranchSize_ &&
						abs(now.branch[1].GetPosition(&storage_) - next.branch[1].GetPosition(&storage_)) < maxBranchSize_);
					now = next;
				}
				else
				{
					return now;
				}
			}

			return source;
		}

		template<class T>
		void PlotPath(const Fork & source, const Fork & sink, std::ostream & out, T & visit) const
		{
			for (int j = 0; j < 2; j++)
			{
				out << source.branch[j].GetVertexId(&storage_) << "[shape=box]" << std::endl;
				out << sink.branch[j].GetVertexId(&storage_) << "[shape=box]" << std::endl;
				for (auto it = source.branch[j]; it != sink.branch[j]; ++it)
				{
					auto jt = it + 1;
					auto pr = std::make_pair(it, jt);
					out << it.GetVertexId(&storage_) << " -> " << jt.GetVertexId(&storage_)
						<< "[label=\"" << it.GetChar(&storage_) << ", " << it.GetChrId() << ", " << it.GetPosition(&storage_) << ", " << abs(it.GetPosition(&storage_) - jt.GetPosition(&storage_)) << "\""
						<< (it.IsPositiveStrand() ? "color=lightskyblue" : "color=orange") << "]\n";
					visit.push_back(pr);
				}
			}
		}
		
		Fork TakeBubbleStep(const Fork & source) const
		{
			auto it = source.branch[0];
			std::map<int64_t, int64_t> firstBranch;
			for (int64_t i = 1; abs(it.GetPosition(&storage_) - source.branch[0].GetPosition(&storage_)) < maxBranchSize_ && (++it).Valid(&storage_); i++)
			{
				int64_t d = abs(it.GetPosition(&storage_) - source.branch[0].GetPosition(&storage_));
				firstBranch[it.GetVertexId(&storage_)] = i;
			}

			it = source.branch[1];			
			for (int64_t i = 1; abs(it.GetPosition(&storage_) - source.branch[1].GetPosition(&storage_)) < maxBranchSize_ && (++it).Valid(&storage_); i++)
			{
				auto kt = firstBranch.find(it.GetVertexId(&storage_));
				if (kt != firstBranch.end())
				{
					return Fork(source.branch[0] + kt->second, it);
				}
			}

			return Fork(JunctionStorage::JunctionIterator(), JunctionStorage::JunctionIterator());
		}

		void BubbledBranchesForward(int64_t vertexId, const std::vector<JunctionStorage::JunctionIterator> & instance, BubbledBranches & bulges) const
		{						
			std::vector<size_t> parallelEdge[4];
			std::map<int64_t, BranchData> visit;
			bulges.assign(instance.size(), std::vector<size_t>());
			for (size_t i = 0; i < instance.size(); i++)
			{
				auto vertex = instance[i];
				if((vertex + 1).Valid(&storage_))
				{
					parallelEdge[TwoPaCo::DnaChar::MakeUpChar(vertex.GetChar(&storage_))].push_back(i);
				}
				
				for (int64_t startPosition = vertex++.GetPosition(&storage_); vertex.Valid(&storage_) && abs(startPosition - vertex.GetPosition(&storage_)) <= maxBranchSize_; ++vertex)
				{
					int64_t nowVertexId = vertex.GetVertexId(&storage_);
					auto point = visit.find(nowVertexId);
					if (point == visit.end())
					{
						BranchData bData;
						bData.branchId.push_back(i);
						visit[nowVertexId] = bData;
					}
					else
					{
						point->second.branchId.push_back(i);
					}
				}
			}

			for (size_t i = 0; i < 4; i++)
			{
				for (size_t j = 0; j < parallelEdge[i].size(); j++)
				{
					for (size_t k = j + 1; k < parallelEdge[i].size(); k++)
					{
						size_t smallBranch = parallelEdge[i][j];
						size_t largeBranch = parallelEdge[i][k];
						bulges[smallBranch].push_back(largeBranch);
					}
				}
			}

			for (auto point = visit.begin(); point != visit.end(); ++point)
			{
				std::sort(point->second.branchId.begin(), point->second.branchId.end());
				for (size_t j = 0; j < point->second.branchId.size(); j++)
				{
					for (size_t k = j + 1; k < point->second.branchId.size(); k++)
					{
						size_t smallBranch = point->second.branchId[j];
						size_t largeBranch = point->second.branchId[k];
						if (smallBranch != largeBranch && std::find(bulges[smallBranch].begin(), bulges[smallBranch].end(), largeBranch) == bulges[smallBranch].end())
						{
							bulges[smallBranch].push_back(largeBranch);
						}						
					}
				}
			}
		}

		void BubbledBranchesBackward(int64_t vertexId, const std::vector<JunctionStorage::JunctionIterator> & instance, BubbledBranches & bulges) const
		{
			std::vector<size_t> parallelEdge[4];
			std::map<int64_t, BranchData> visit;
			bulges.assign(instance.size(), std::vector<size_t>());
			for (size_t i = 0; i < instance.size(); i++)
			{
				auto vertex = instance[i];
				auto prev = vertex - 1;
				if (prev.Valid(&storage_))
				{
					parallelEdge[TwoPaCo::DnaChar::MakeUpChar(prev.GetChar(&storage_))].push_back(i);
				}
				
				for (int64_t startPosition = vertex--.GetPosition(&storage_); vertex.Valid(&storage_) && abs(startPosition - vertex.GetPosition(&storage_)) <= maxBranchSize_; --vertex)
				{
					int64_t nowVertexId = vertex.GetVertexId(&storage_);
					auto point = visit.find(nowVertexId);
					if (point == visit.end())
					{
						BranchData bData;
						bData.branchId.push_back(i);
						visit[nowVertexId] = bData;
					}
					else
					{
						point->second.branchId.push_back(i);
					}
				}
			}

			for (size_t i = 0; i < 4; i++)
			{
				for (size_t j = 0; j < parallelEdge[i].size(); j++)
				{
					for (size_t k = j + 1; k < parallelEdge[i].size(); k++)
					{
						size_t smallBranch = parallelEdge[i][j];
						size_t largeBranch = parallelEdge[i][k];
						bulges[smallBranch].push_back(largeBranch);
					}
				}
			}

			for (auto point = visit.begin(); point != visit.end(); ++point)
			{
				std::sort(point->second.branchId.begin(), point->second.branchId.end());
				for (size_t j = 0; j < point->second.branchId.size(); j++)
				{
					for (size_t k = j + 1; k < point->second.branchId.size(); k++)
					{
						size_t smallBranch = point->second.branchId[j];
						size_t largeBranch = point->second.branchId[k];
						if (smallBranch != largeBranch && std::find(bulges[smallBranch].begin(), bulges[smallBranch].end(), largeBranch) == bulges[smallBranch].end())
						{
							bulges[smallBranch].push_back(largeBranch);
						}
					}
				}
			}
		}		


		int64_t k_;
		int64_t minBlockSize_;
		int64_t maxBranchSize_;
		std::vector<Fork> sink_;
		std::vector<Fork> source_;
		JunctionStorage & storage_;		
		std::vector<std::vector<Edge> > syntenyPath_;
		std::vector<std::vector<Assignment> > blockId_;	
		std::string debugOut_;
	};
}

#endif