#pragma once

#include "Windows.h"
#include "Traffic.h"

namespace dr {
	class Drawing
	{
	private:
		double scale;
		RECT drawRect;

	public:
		Drawing();
		Drawing(tr::Traffic& traffic, RECT& drawRect);
		void Init(tr::Traffic& traffic, RECT& drawRect);
		void DrawPath(HDC hdc, tr::Traffic& traffic, int selStation);
		void DrawMark(HDC hdc, tr::Traffic& traffic, int bus_id, bool selected);
	};
}