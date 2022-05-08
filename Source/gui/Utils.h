#pragma once
#include "Using.h"
#include "Shared.h"
#include "Events.h"
#include "../audio/MIDILearn.h"

namespace gui
{
	using Notify = evt::Notify;
	using Evt = evt::System::Evt;
	using EvtType = evt::Type;
	using EventSystem = evt::System;

	enum class CursorType
	{
		Default,
		Interact,
		Inactive,
		Mod,
		Bias,
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
		else if (c == CursorType::Bias)
			col = Colours::c(ColourID::Bias);

		for (auto y = 0; y < h; ++y)
			for (auto x = 0; x < w; ++x)
				if (img.getPixelAt(x, y) == imgCol)
					img.setPixelAt(x, y, col);

		static constexpr int scale = 3;
		img = img.rescaled(w * scale, h * scale, Graphics::ResamplingQuality::lowResamplingQuality);

		return { img, 0, 0 };
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
		
		std::vector<Param*>& getAllParams() noexcept
		{
			return params.data();
		}
		const std::vector<Param*>& getAllParams() const noexcept
		{
			return params.data();
		}

		juce::ValueTree getState() const noexcept
		{
			return audioProcessor.state.getState();
		}

		void assignMIDILearn(PID pID) noexcept
		{
			audioProcessor.midiLearn.assignParam(params[pID]);
		}
		void removeMIDILearn(PID pID) noexcept
		{
			audioProcessor.midiLearn.removeParam(params[pID]);
		}
		const audio::MIDILearn& getMIDILearn() const noexcept
		{
			return audioProcessor.midiLearn;
		}

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
			auto t = thicc(pluginTop.getLocalBounds()) * .1f;
			return t < 1.f ? 1.f : t;
		}

		float fontHeight() const noexcept
		{
			const auto w = static_cast<float>(pluginTop.getWidth());
			const auto h = static_cast<float>(pluginTop.getHeight());

			const auto avr = (w + h) * .5f;
			const auto norm = (avr - 500.f) / 500.f;
			return std::floor(8.5f + norm * 5.f);
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