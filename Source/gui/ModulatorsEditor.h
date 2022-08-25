#pragma once
#include "Comp.h"

namespace gui
{
	struct ModulatorsComp :
		public Comp
	{
		ModulatorsComp(Utils& u) :
			Comp(u, "", CursorType::Default)
		{}
	};
}