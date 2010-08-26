#ifndef _MEMDC_H_ // [
#define _MEMDC_H_

//////////////////////////////////////////////////
// CMemDC - memory DC
//
// Author: Keith Rule
// Email:  keithr@europa.com
// Copyright 1996-2002, Keith Rule
//
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
// History - 10/3/97 Fixed scrolling bug.
//               Added print support. - KR
//
//       11/3/99 Fixed most common complaint. Added
//            background color fill. - KR
//
//       11/3/99 Added support for mapping modes other than
//            MM_TEXT as suggested by Lee Sang Hun. - KR
//
//       02/11/02 Added support for CScrollView as supplied
//             by Gary Kirkham. - KR
//
// This class implements a memory Device Context which allows
// flicker free drawing.

class CMemDC : public CDC
{
private:
	CBitmap    m_bitmap;    // Offscreen bitmap
	CBitmap    m_bitmap1;   // Monochrome Offscreen bitmap
	CBitmap   *m_oldBitmap; // bitmap originally found in CMemDC
	CDC*       m_pDC;       // Saves CDC passed in constructor
	CRect      m_rect;      // Rectangle of drawing area.
	BOOL       m_bMemDC;    // TRUE if CDC really is a Memory DC.

public:
	CMemDC(CDC* pDC, const CRect* pRect = NULL);
	~CMemDC();

	void mono();
	void compatible();

	// Allow usage as a pointer
	CMemDC* operator->()
	{
		return this;
	}

	// Allow usage as a pointer
	operator CMemDC*()
	{
		return this;
	}
};

#endif // ]
