#pragma once
#include "Param.h"
#include "Events.h"
#include "Processor.h"

namespace gui
{
	using Component = juce::Component;
	using Processor = audio::Processor;

	using Notify = evt::Notify;
	using Evt = evt::System::Evt;
	using EvtType = evt::Type;

	using PID = param::PID;
	using Param = param::Param;

	static constexpr float Tau = 6.28318530718f;
	static constexpr float Pi = Tau * .5f;;
	static constexpr float PiHalf = Tau * .25f;
	static constexpr float PiQuart = Tau * .125f;

	using Mouse = juce::MouseEvent;
	using Graphics = juce::Graphics;
	using Just = juce::Justification;
	using Timer = juce::Timer;
	using Path = juce::Path;
	using Point = juce::Point<int>;
	using PointF = juce::Point<float>;
	using String = juce::String;
	using LineF = juce::Line<float>;

	enum class CursorType
	{
		Default,
		Interact,
		Inactive,
		Mod,
		NumTypes
	};

	inline juce::MouseCursor makeCursor(CursorType c)
	{
		Image img = juce::ImageCache::getFromMemory(BinaryData::cursor_png, BinaryData::cursor_pngSize).createCopy();

		const auto w = img.getWidth();
		const auto h = img.getHeight();

		const Colour imgCol(0xff37946e);

		Colour col;

		if (c == CursorType::Default)
			col = Colours::c(ColourID::Txt);
		else if (c == CursorType::Interact)
			col = Colours::c(ColourID::Interact);
		else if (c == CursorType::Inactive)
			col = Colours::c(ColourID::Inactive);
		else if (c == CursorType::Mod)
			col = Colours::c(ColourID::Mod);

		for (auto y = 0; y < h; ++y)
			for (auto x = 0; x < w; ++x)
				if (img.getPixelAt(x, y) == imgCol)
					img.setPixelAt(x, y, col);

		static constexpr int scale = 3;
		img = img.rescaled(w * scale, h * scale, Graphics::ResamplingQuality::lowResamplingQuality);

		return { img, 0, 0 };
	}

	inline void hideCursor()
	{
		auto& desktop = juce::Desktop::getInstance();
		auto mainMouse = desktop.getMainMouseSource();
		mainMouse.hideCursor();
	}

	inline void showCursor()
	{
		auto& desktop = juce::Desktop::getInstance();
		auto mainMouse = desktop.getMainMouseSource();
		mainMouse.revealCursor();
	}

	class Utils
	{
		Notify makeNotify(Utils* u)
		{
			return [u](const EvtType, const void*)
			{
				
			};
		}

		static constexpr float DragSpeed = .5f;

		using Params = param::Params;
		using EventSystem = evt::System;
		using AppProps = juce::ApplicationProperties;
	public:

		Utils(Component& _pluginTop, Processor& _audioProcessor) :
			pluginTop(_pluginTop),
			audioProcessor(_audioProcessor),
			params(audioProcessor.params),
			eventSystem(),
			evt(eventSystem, makeNotify(this))
		{
			Colours::c.init(audioProcessor.props.getUserSettings());
		}

		Param* getParam(PID pID) noexcept { return params[pID]; }
		const Param* getParam(PID pID) const noexcept { return params[pID]; }

		float getDragSpeed() const noexcept
		{
			const auto height = static_cast<float>(pluginTop.getHeight());
			const auto speed = DragSpeed * height;
			return speed;
		}

		float thicc(int widthOrHeight) const noexcept
		{
			return static_cast<float>(widthOrHeight) * .05f;
		}
		float thicc(Bounds bounds) const noexcept
		{
			return thicc(std::min(bounds.getWidth(), bounds.getHeight()));
		}
		float thicc() const noexcept
		{
			const auto t = thicc(pluginTop.getLocalBounds()) * .1f;
			return t < 1.f ? 1.f : t;
		}

		EventSystem& getEventSystem()
		{
			return eventSystem;
		}
	
		const std::atomic<float>& getMeter(int i) const noexcept
		{
			return audioProcessor.meters(i);
		}

		Point getScreenPosition() const noexcept { return pluginTop.getScreenPosition(); }

		Component& pluginTop;
	protected:
		Processor& audioProcessor;
		Params& params;
		EventSystem eventSystem;
		Evt evt;
	};
}