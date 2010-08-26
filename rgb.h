#pragma once

#include "stdafx.h"

struct _C_CONV_COLOR_REF
{
	union
	{
		COLORREF cr;
		struct
		{
			BYTE r,g,b;
		};
	};

	_C_CONV_COLOR_REF(COLORREF _cr) : cr(_cr) {}
};

class C_RGB
{
	void setFrom( COLORREF c ) 
	{
		_C_CONV_COLOR_REF ccr(c); 
		r=ccr.r; 
		g=ccr.g; 
		b=ccr.b; 
	}

public: // Data
	typedef BYTE rgb_t;

	union
	{
		DWORD rgb;
		struct
		{
			rgb_t b,g,r;
		};
	};

public: // constructors
	C_RGB() : rgb(0) {}
	C_RGB(C_RGB const &from) : rgb(from.rgb) {}
	C_RGB(COLORREF c) { setFrom(c); }
	C_RGB(rgb_t R = 0, rgb_t G = 0, rgb_t B = 0) : r(R), g(G), b(B) {}

	void Set(rgb_t R = 0, rgb_t G = 0, rgb_t B = 0)
	{
		r = R;
		g = G;
		b = B;
	}

public: 
	// Masquerade as COLORREF type
	operator COLORREF () const { return RGB(r,g,b); }
	C_RGB &operator = ( COLORREF c ) { setFrom(c); return *this; }

	C_RGB &operator = ( C_RGB const &from ) { rgb = from.rgb; return *this; }

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


class C_RGBA
{
	void setFrom( COLORREF c ) 
	{
		_C_CONV_COLOR_REF ccr(c); 
		r=ccr.r; 
		g=ccr.g; 
		b=ccr.b; 
	}

public:
	typedef BYTE rgba_t;

	union
	{
		DWORD rgba;
		struct
		{
			rgba_t b,g,r,a;
		};
	};

	C_RGBA() : rgba(0) {}
	C_RGBA(C_RGBA const &from) : rgba(from.rgba) {}
	C_RGBA(COLORREF c) { setFrom(c); }

	// Masquerade as COLORREF type
	operator COLORREF() const { return RGB(r,g,b); }
	C_RGBA &operator = ( COLORREF c ) { setFrom(c); return *this; }

	C_RGBA &operator = ( C_RGBA const &from ) { rgba = from.rgba;  return *this; }
};
