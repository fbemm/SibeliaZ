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

			std::ofstream debugStream(debugOut.c_str());						
			time_t mark = time(0);

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
		
		void BubbledBranchesForward(int64_t vertexId, const std::vector<JunctionStorage::JunctionIterator> & instance, BubbledBranches & bulges) const
		{						
			std::vector<size_t> parallelEdge[4];
			std::map<int64_t, BranchData> visit;
			bulges.assign(instance.size(), std::vector<size_t>());
			for (size_t i = 0; i < instance.size(); i++)
			{
				auto vertex = instance[i];
				parallelEdge[TwoPaCo::DnaChar::MakeUpChar(vertex.GetChar(&storage_))].push_back(i);
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
						break;
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

		void BubbledBranchesForward(int64_t vertexId, const std::vector<JunctionStorage::JunctionIterator> & instance, BubbledBranches & bulges) const
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
						break;
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
		JunctionStorage & storage_;
		std::vector<std::vector<Edge> > syntenyPath_;
		std::vector<std::vector<Assignment> > blockId_;	
		std::string debugOut_;
	};
}

#endif