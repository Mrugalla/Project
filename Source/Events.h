#pragma once
#include <juce_events/juce_events.h>
#include <vector>
#include <functional>

namespace evt
{
    enum class Type
    {
        ColourShemeChanged,
        TooltipUpdated,
        ButtonClicked,
        ParametrRightClicked,
        ParametrDragged,
        ClickedEmpty,
        PatchUpdated,
        ComponentAdded,
        NumTypes
    };

    using Notify = std::function<void(const Type, const void*)>;

	class System
	{
        using Mutex = juce::CriticalSection;
        using Lock = juce::ScopedLock;
    public:
		struct Evt
		{
            Evt(System& _sys) :
                sys(_sys),
                notifier(nullptr)
            {
            }
            Evt(System& _sys, const Notify& _notifier) :
                sys(_sys),
                notifier(_notifier)
            {
                sys.add(this);
            }
            Evt(System& _sys, Notify&& _notifier) :
                sys(_sys),
                notifier(_notifier)
            {
                sys.add(this);
            }

            ~Evt()
            {
                sys.remove(this);
            }

            void operator()(const Type type, const void* stuff = nullptr) const
            {
                sys.notify(type, stuff);
            }

            Notify notifier;
        protected:
            System& sys;
		};

		System() :
			evts(),
            mutex()
		{}

        void notify(const Type type, const void* stuff = nullptr)
        {
            const Lock lock(mutex);
            for (const auto e : evts)
                e->notifier(type, stuff);
        }
    protected:
		std::vector<Evt*> evts;

        void add(Evt* e)
        {
            const Lock lock(mutex);
            evts.push_back(e);
        }
        
        void remove(const Evt* e)
        {
            const Lock lock(mutex);
            for (auto i = 0; i < evts.size(); ++i)
                if (e == evts[i])
                {
                    evts.erase(evts.begin() + i);
                    return;
                }
        }
    private:
        Mutex mutex;
	};
}