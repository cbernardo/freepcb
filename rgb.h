#pragma once

#include "stdafx.h"

class C_RGB
{
public: // Data
	typedef BYTE rgb_t;

	rgb_t r,g,b;

public: // constructors
	C_RGB(rgb_t R = 0, rgb_t G = 0, rgb_t B = 0) : r(R), g(G), b(B) {}
	C_RGB(C_RGB const &from)                     : r(from.r), g(from.g), b(from.b) {}

	void Set(rgb_t R = 0, rgb_t G = 0, rgb_t B = 0)
	{
		r = R;
		g = G;
		b = B;
	}

public: // Masquerade as COLORREF type
	operator COLORREF () const
	{
		return RGB(r,g,b);
	}

	C_RGB &operator = ( COLORREF color )
	{
		r = GetRValue( color );
		g = GetGValue( color );
		b = GetBValue( color );

		return *this;
	}

	// Colors
	static const COLORREF black      = RGB(0,   0,   0   );
	static const COLORREF white      = RGB(255, 255, 255 );
	static const COLORREF grey       = RGB(128, 128, 128 );
	static const COLORREF gray       = grey;
	static const COLORREF red        = RGB(255, 0,   0   );
	static const COLORREF green      = RGB(0,   255, 0   );
	static const COLORREF blue       = RGB(0,   0,   255 );
	static const COLORREF dark_red   = RGB(127, 0,   0   );
	static const COLORREF dark_green = RGB(0,   127, 0   );
	static const COLORREF dark_blue  = RGB(0,   0,   127 );
	static const COLORREF yellow     = RGB(255, 255, 0   );
	static const COLORREF violet     = RGB(255, 0,   255 );
	static const COLORREF orange     = RGB(255, 128, 64  );

	static const COLORREF mono_off   = black;
	static const COLORREF mono_on    = white;
};