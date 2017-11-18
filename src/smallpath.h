#ifndef _SMALL_PATH_H_
#define _SMALL_PATH_H_

#include <set>
#include <atomic>
#include <cassert>
#include <algorithm>
#include "distancekeeper.h"

#include <tbb/blocked_range.h>

namespace Sibelia
{
	struct SmallPath
	{
	public:
		SmallPath(const JunctionStorage & storage,
			int64_t maxBranchSize,
			int64_t minBlockSize,
			int64_t maxFlankingSize) :
			maxBranchSize_(maxBranchSize), minBlockSize_(minBlockSize), maxFlankingSize_(maxFlankingSize), storage_(&storage), minChainSize_(minBlockSize - 2 * maxFlankingSize)
		{

		}

		void Init(int64_t vid)
		{
			origin_ = vid;
			for (size_t i = 0; i < storage_->GetInstancesCount(vid); i++)
			{
				JunctionStorage::JunctionIterator it = storage_->GetJunctionInstance(vid, i);
				if (!it.IsUsed(storage_))
				{
					instance_.insert(Instance(it, 0));
				}
			}
		}

		
		struct Instance
		{
		private:
			int64_t frontDistance_;
			int64_t backDistance_;
			JunctionStorage::JunctionIterator front_;
			JunctionStorage::JunctionIterator back_;
		public:

			Instance()
			{

			}

			Instance(JunctionStorage::JunctionIterator & it, int64_t distance) : front_(it), back_(it), frontDistance_(distance), backDistance_(distance)
			{

			}

			void ChangeFront(const JunctionStorage::JunctionIterator & it, int64_t distance)
			{
				front_ = it;
				frontDistance_ = distance;
			}

			void ChangeBack(const JunctionStorage::JunctionIterator & it, int64_t distance)
			{
				back_ = it;
				backDistance_ = distance;
			}

			bool SinglePoint() const
			{
				return front_ == back_;
			}

			JunctionStorage::JunctionIterator Front() const
			{
				return front_;
			}

			JunctionStorage::JunctionIterator Back() const
			{
				return back_;
			}

			int64_t LeftFlankDistance() const
			{
				return frontDistance_;
			}

			int64_t RightFlankDistance() const
			{
				return backDistance_;
			}

			bool Within(const JunctionStorage::JunctionIterator it) const
			{
				if (it.GetChrId() == front_.GetChrId())
				{
					int64_t left = min(front_.GetIndex(), back_.GetIndex());
					int64_t right = max(front_.GetIndex(), back_.GetIndex());
					return it.GetIndex() >= left && it.GetIndex() <= right;
				}

				return false;
			}

			bool operator < (const Instance & inst) const
			{
				if (front_.GetChrId() != inst.front_.GetChrId())
				{
					return front_.GetChrId() < inst.front_.GetChrId();
				}

				int64_t idx1 = back_.IsPositiveStrand() ? back_.GetIndex() : front_.GetIndex();
				int64_t idx2 = inst.back_.IsPositiveStrand() ? inst.back_.GetIndex() : inst.front_.GetIndex();
				return idx1 < idx2;
			}
		};

		struct Point
		{
		private:
			Edge edge;
			int64_t startDistance;
		public:
			Point() {}
			Point(Edge edge, int64_t startDistance) : edge(edge), startDistance(startDistance) {}

			Edge GetEdge() const
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

		int64_t Origin() const
		{
			return origin_;
		}

		const std::multiset<Instance> & Instances() const
		{
			return instance_;
		}

		int64_t MiddlePathLength() const
		{
			return (rightBody_.size() > 0 ? rightBody_.back().EndDistance() : 0) - (leftBody_.size() > 0 ? leftBody_.back().StartDistance() : 0);
		}

		int64_t GetEndVertex() const
		{
			if (rightBody_.size() > 0)
			{
				return rightBody_.back().GetEdge().GetEndVertex();
			}

			return origin_;
		}

		int64_t GetStartVertex() const
		{
			if (leftBody_.size() > 0)
			{
				return leftBody_.back().GetEdge().GetStartVertex();
			}

			return origin_;
		}

		void DumpInstances(std::ostream & out) const
		{
			for (auto inst : instance_)
			{
				out << "(" << (inst.Front().IsPositiveStrand() ? '+' : '-') << inst.Front().GetChrId() << ' ' << inst.Front().GetIndex() << ' ' << inst.Back().GetIndex() << ')' << std::endl;
			}
		}

		void DumpPath(std::vector<Edge> & ret) const
		{
			ret.clear();
			for (auto it = leftBody_.rbegin(); it != leftBody_.rend(); ++it)
			{
				ret.push_back(it->GetEdge());
			}

			for (auto it = rightBody_.rbegin(); it != rightBody_.rend(); ++it)
			{
				ret.push_back(it->GetEdge());
			}
		}

		bool Compatible(const JunctionStorage::JunctionIterator & start, const JunctionStorage::JunctionIterator & end, const Edge & e) const
		{
			if (start.GetChrId() != end.GetChrId() || start.IsPositiveStrand() != end.IsPositiveStrand())
			{
				return false;
			}

			int64_t diff = end.GetPosition(storage_) - start.GetPosition(storage_);
			if (start.IsPositiveStrand())
			{
				if (diff < 0)
				{
					return false;
				}

				auto start1 = start + 1;
				if (diff > maxBranchSize_ && (start.GetChar(storage_) != e.GetChar() || end != start1 || start1.GetVertexId(storage_) != e.GetEndVertex()))
				{
					return false;
				}
			}
			else
			{
				if (-diff < 0)
				{
					return false;
				}

				auto start1 = start + 1;
				if (-diff > maxBranchSize_ && (start.GetChar(storage_) != e.GetChar() || end != start1 || start1.GetVertexId(storage_) != e.GetEndVertex()))
				{
					return false;
				}
			}

			return true;
		}

		class PointPushFrontWorker
		{
		public:
			Edge e;
			SmallPath * path;
			int64_t vertex;
			int64_t distance;
			bool & failFlag;

			PointPushFrontWorker(SmallPath * path, int64_t vertex, int64_t distance, Edge e, bool & failFlag) : path(path), vertex(vertex), e(e), failFlag(failFlag), distance(distance)
			{

			}

			void operator()(const tbb::blocked_range<size_t> & range) const
			{
				for (size_t i = range.begin(); i != range.end() && !failFlag; i++)
				{
					bool newInstance = true;
					JunctionStorage::JunctionIterator nowIt = path->storage_->GetJunctionInstance(vertex, i);
					if (!nowIt.IsUsed(path->storage_))
					{
						auto inst = path->instance_.upper_bound(Instance(nowIt, 0));
						if (inst != path->instance_.end() && inst->Within(nowIt))
						{
							continue;
						}

						if (nowIt.IsPositiveStrand())
						{
							if (inst != path->instance_.end() && path->Compatible(nowIt, inst->Front(), e))
							{
								newInstance = false;
							}
						}
						else
						{
							if (inst != path->instance_.begin() && path->Compatible(nowIt, (--inst)->Front(), e))
							{
								newInstance = false;
							}
						}

						if (!newInstance && inst->Front().GetVertexId(path->storage_) != vertex)
						{
							int64_t nextLength = abs(nowIt.GetPosition(path->storage_) - inst->Back().GetPosition(path->storage_));
							int64_t rightFlankSize = abs(inst->RightFlankDistance() - (path->rightBody_.empty() ? 0 : path->rightBody_.back().EndDistance()));
							if (nextLength >= path->minChainSize_ && rightFlankSize > path->maxFlankingSize_)
							{
								failFlag = true;
								break;
							}

							const_cast<Instance&>(*inst).ChangeFront(nowIt, distance);
						}
						else
						{
							path->instance_.insert(Instance(nowIt, distance));
						}
					}
				}
			}

		};

		class PointPushBackWorker
		{
		public:
			Edge e;
			SmallPath * path;
			int64_t vertex;
			int64_t distance;
			bool & failFlag;

			PointPushBackWorker(SmallPath * path, int64_t vertex, int64_t distance, Edge e, bool & failFlag) : path(path), vertex(vertex), e(e), failFlag(failFlag), distance(distance)
			{

			}

			void operator()(const tbb::blocked_range<size_t> & range) const
			{
				for (size_t i = range.begin(); i < range.end() && !failFlag; i++)
				{
					bool newInstance = true;
					JunctionStorage::JunctionIterator nowIt = path->storage_->GetJunctionInstance(vertex, i);
					if (!nowIt.IsUsed(path->storage_))
					{
						auto inst = path->instance_.upper_bound(Instance(nowIt, 0));
						if (inst != path->instance_.end() && inst->Within(nowIt))
						{
							continue;
						}

						if (nowIt.IsPositiveStrand())
						{
							if (inst != path->instance_.begin() && path->Compatible((--inst)->Back(), nowIt, e))
							{
								newInstance = false;
							}
						}
						else
						{
							if (inst != path->instance_.end() && path->Compatible(inst->Back(), nowIt, e))
							{
								newInstance = false;
							}
						}

						if (!newInstance && inst->Back().GetVertexId(path->storage_) != vertex)
						{
							int64_t nextLength = abs(nowIt.GetPosition(path->storage_) - inst->Front().GetPosition(path->storage_));
							int64_t leftFlankSize = abs(inst->LeftFlankDistance() - (path->leftBody_.empty() ? 0 : path->leftBody_.back().StartDistance()));
							if (nextLength >= path->minChainSize_ && leftFlankSize > path->maxFlankingSize_)
							{
								failFlag = true;
								break;
							}

							const_cast<Instance&>(*inst).ChangeBack(nowIt, distance);
						}
						else
						{
							path->instance_.insert(Instance(nowIt, distance));
						}
					}
				}
			}

		};

		bool PointPushBack(const Edge & e)
		{
			int64_t vertex = e.GetEndVertex();
			bool failFlag;
			failFlag = false;
			int64_t startVertexDistance = rightBody_.empty() ? 0 : rightBody_.back().EndDistance();
			int64_t endVertexDistance = startVertexDistance + e.GetLength();
			PointPushBackWorker(this, vertex, endVertexDistance, e, failFlag)(tbb::blocked_range<size_t>(0, storage_->GetInstancesCount(vertex)));
			rightBody_.push_back(Point(e, startVertexDistance));
			return !failFlag;
		}

		
		bool PointPushFront(const Edge & e)
		{
			int64_t vertex = e.GetStartVertex();			
			bool failFlag;
			failFlag = false;
			int64_t endVertexDistance = leftBody_.empty() ? 0 : leftBody_.back().StartDistance();
			int64_t startVertexDistance = endVertexDistance - e.GetLength();
			PointPushFrontWorker(this, vertex, startVertexDistance, e, failFlag)(tbb::blocked_range<size_t>(0, storage_->GetInstancesCount(vertex)));
			leftBody_.push_back(Point(e, startVertexDistance));
			return !failFlag;
		}		

		int64_t Score(bool final = false) const
		{
			int64_t score;
			int64_t length;
			int64_t ret = 0;
			for (auto & inst : instance_)
			{
				InstanceScore(inst, length, score);
				if (!final || length >= minChainSize_)
				{
					ret += score;
				}
			}

			return ret;
		}

		int64_t GoodInstances() const
		{
			int64_t ret = 0;
			for (auto & it : instance_)
			{
				if (IsGoodInstance(it))
				{
					ret++;
				}
			}

			return ret;
		}

		bool IsGoodInstance(const Instance & it) const
		{
			int64_t score;
			int64_t length;
			InstanceScore(it, length, score);
			return length >= minChainSize_;
		}

		void InstanceScore(const Instance & inst, int64_t & length, int64_t & score) const
		{
			int64_t leftFlank = abs(inst.LeftFlankDistance() - (leftBody_.size() > 0 ? leftBody_.back().StartDistance() : 0));
			int64_t rightFlank = abs(inst.RightFlankDistance() - (rightBody_.size() > 0 ? rightBody_.back().EndDistance() : 0));
			length = abs(inst.Front().GetPosition(storage_) - inst.Back().GetPosition(storage_));
			score = length - leftFlank - rightFlank;
		}

		void Clear()
		{
			leftBody_.clear();
			rightBody_.clear();
			instance_.clear();
		}

	private:



		std::vector<Point> leftBody_;
		std::vector<Point> rightBody_;
		std::multiset<Instance> instance_;

		int64_t origin_;
		int64_t minChainSize_;
		int64_t minBlockSize_;
		int64_t maxBranchSize_;
		int64_t maxFlankingSize_;
		const JunctionStorage * storage_;
	};

	struct SmallBestPath
	{
		int64_t score;
		tbb::mutex mutex_;
		std::vector<SmallPath::Instance> instance;

		void Init()
		{
			score = 0;
			instance.clear();
		}

		void Clear()
		{
			Init();
		}

		void Update(const SmallPath & path, int64_t minBlockSize)
		{
			tbb::mutex::scoped_lock lock(mutex_);
			int64_t newScore = path.Score(true);
			if (newScore > score && path.MiddlePathLength() >= minBlockSize)
			{
				score = path.Score(true);
				instance.clear();
				for (auto inst : path.Instances())
				{
					if (path.IsGoodInstance(inst))
					{
						instance.push_back(inst);
					}
				}
			}
		}
	};

}

#endif

