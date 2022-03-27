#pragma once
#include <juce_events/juce_events.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <atomic>

namespace rtp
{
	using Timer = juce::Timer;

	class ReleasePool :
		Timer
	{
		using Pool = std::vector<std::shared_ptr<void>>;
		using Mutex = juce::CriticalSection;
		using Lock = juce::ScopedLock;
		static constexpr int MinPtrCount = 2;

	public:
		ReleasePool() :
			pool(),
			mutex()
		{}

		template<typename T>
		void add(const std::shared_ptr<T>& ptr)
		{
			const Lock lock(mutex);
			
			if (ptr == nullptr)
				return;
			if (existsAlready(ptr))
				return;
			
			pool.emplace_back(ptr);
			
			if (!isTimerRunning())
				startTimer(10000);
		}

		template<typename T>
		void remove(const std::shared_ptr<T>& ptr)
		{
			for (auto p = 0; p < pool.size(); ++p)
				if (pool[p] == ptr)
				{
					pool.erase(pool.begin() + p);
					return;
				}
		}

		void release()
		{
			const Lock lock(mutex);
			pool.erase(
				std::remove_if(
					pool.begin(), pool.end(), [](auto& object) { return object.use_count() < MinPtrCount; }),
				pool.end());
		}

		static ReleasePool rp;
	protected:
		Pool pool;
		Mutex mutex;

		void timerCallback() override
		{
			release();
		}

		template<typename T>
		bool existsAlready(const std::shared_ptr<T>& ptr)
		{
			for (const auto& p : pool)
				if (p.get() == ptr.get())
					return true;
			return false;
		}
	};

	template<class T>
	struct RealTimePtr
	{
		using Ptr = std::shared_ptr<T>;

		RealTimePtr(T&& args = {}) :
			ptr(std::make_shared<T>(args))
		{
			ReleasePool::rp.add(ptr);
		}

		~RealTimePtr()
		{
			ReleasePool::rp.remove(ptr);
		}

		T* operator->()
		{
			return ptr.get();
		}

		T* operator*()
		{
			return ptr.get();
		}

		Ptr get()
		{
			return ptr;
		}

		Ptr load()
		{
			return std::atomic_load(&ptr);
		}

		Ptr withCopy()
		{
			return std::make_shared<T>(std::atomic_load(&ptr));
		}

		void replaceWith(const Ptr& nPtr)
		{
			ReleasePool::rp.add(nPtr);
			std::atomic_store(&ptr, nPtr);
		}

	protected:
		Ptr ptr;
	};
}