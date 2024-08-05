#include "drawing.h"
#define RADIUS 8
#define PADDING 12
#define HEIGHT 30

namespace dr {
	COLORREF green = RGB(0, 128, 0);
	COLORREF orange = RGB(255, 140, 0);
	COLORREF stColor = RGB(100, 0, 155);
	COLORREF selStColor = RGB(0, 255, 255);
}

dr::Drawing::Drawing()
{
	scale = 1;
	drawRect.left = 0;
	drawRect.right = 0;
	drawRect.top = 0;
	drawRect.bottom = 0;
}

dr::Drawing::Drawing(tr::Traffic& traffic, RECT& drawRect)
{
	Init(traffic, drawRect);
}

void dr::Drawing::Init(tr::Traffic& traffic, RECT& drawRect)
{
	this->drawRect = drawRect;
	scale = (drawRect.right - drawRect.left - PADDING * 2) / (double)traffic.Route().TotalLength();
}

void dr::Drawing::DrawPath(HDC hdc, tr::Traffic& traffic, int selStation)
{
	HBRUSH stationBrush = CreateSolidBrush(stColor);
	HBRUSH selStationBrush = CreateSolidBrush(selStColor);
	HPEN thickPen = CreatePen(PS_SOLID, 3, BLACK_PEN);
	HFONT hOldF, hNewF = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Times New Roman Cyr");
	SelectObject(hdc, stationBrush);
	int x = PADDING,
		y = drawRect.bottom / 2;
	
	SelectObject(hdc, thickPen);
	MoveToEx(hdc, x, y, NULL);
	LineTo(hdc, drawRect.right - PADDING * 2, y);

	for (int i = 1; i < traffic.Route().Count(); i++)
	{
		MoveToEx(hdc, x, y, NULL);
		hOldF = (HFONT)SelectObject(hdc, hNewF);
		TextOut(hdc, x - RADIUS / 2, y + HEIGHT / 2, to_string(i - 1).get(), 2);
		SelectObject(hdc, hOldF);
		if (i - 1 == selStation)
			SelectObject(hdc, selStationBrush);
		Ellipse(hdc, x - RADIUS, y - RADIUS, x + RADIUS, y + RADIUS);
		SelectObject(hdc, stationBrush);
		x += round(traffic.Route()(i - 1).Length() * scale);
		//y += delta y;
	}
	MoveToEx(hdc, x, y, NULL);
	hOldF = (HFONT)SelectObject(hdc, hNewF);
	TextOut(hdc, x - RADIUS / 2, y + HEIGHT / 2, to_string(traffic.Route().Count() - 1).get(), 2);
	SelectObject(hdc, hOldF);
	if(selStation == traffic.Route().Count() - 1)
		SelectObject(hdc, selStationBrush);
	Ellipse(hdc, x - RADIUS, y - RADIUS, x + RADIUS, y + RADIUS);
	
	DeleteObject(stationBrush);
	DeleteObject(thickPen);
	DeleteObject(selStationBrush);
	DeleteObject(hNewF);
}

void dr::Drawing::DrawMark(HDC hdc, tr::Traffic& traffic, int bus_id, bool selected)
{
	if (bus_id < 0)
		return;
	int x = PADDING + traffic.Roster(bus_id).GlobCoord() * scale,
		y = drawRect.bottom / 2;
	
	COLORREF markColor = RGB(0,0,0);
	switch (traffic.Roster(bus_id).Status())
	{
	case 0:
		markColor = RGB(255,255,255);
		break;
	case 1:
		markColor = green;
		break;
	case 2:
		markColor = orange;
		break;
	}
	HBRUSH markBrush = CreateSolidBrush(markColor);
	SelectObject(hdc, markBrush);
	HPEN pen = CreatePen(PS_SOLID, selected ? 2 : 1, BLACK_PEN);
	SelectObject(hdc, pen);
	
	if (traffic.Roster(bus_id).Depart() < traffic.Roster(bus_id).Dest()) //	едет вправо - рисуем сверху
	{
		POINT triangle[] = { {x - RADIUS, y - HEIGHT}, {x + RADIUS - 1, y - HEIGHT}, {x, y} };
		Polygon(hdc, triangle, 3);
		Ellipse(hdc, x - RADIUS, y - RADIUS - HEIGHT, x + RADIUS, y + RADIUS - HEIGHT);
	}
	else																//	иначе - снизу
	{
		POINT triangle[] = { {x - RADIUS, y + HEIGHT}, {x + RADIUS - 1, y + HEIGHT}, {x, y} };
		Polygon(hdc, triangle, 3);
		Ellipse(hdc, x - RADIUS, y - RADIUS + HEIGHT, x + RADIUS, y + RADIUS + HEIGHT);
	}

	DeleteObject(markBrush);
	DeleteObject(pen);
}


#undef RADIUS
#undef PADDING
#undef HEIGHT