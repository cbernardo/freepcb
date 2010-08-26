#include "stdafx.h"
#include "memdc.h"

CMemDC::CMemDC(CDC* pDC, const CRect* pRect) : CDC()
{
	ASSERT(pDC != NULL);

	// Some initialization
	m_pDC = pDC;
	m_oldBitmap = NULL;
	m_bMemDC = !pDC->IsPrinting();

	// Get the rectangle to draw
	if (pRect == NULL)
	{
		pDC->GetClipBox(&m_rect);
	}
	else
	{
		m_rect = *pRect;
	}

	if (m_bMemDC)
	{
		// Create a Memory DC
		CreateCompatibleDC(pDC);

		SetBkColor( pDC->GetBkColor() );

		SetMapMode(pDC->GetMapMode());
		SetWindowExt(pDC->GetWindowExt());
		SetViewportExt(pDC->GetViewportExt());

		SetWindowOrg(m_rect.left, m_rect.top);
	}
	else
	{
		// Make a copy of the relevent parts of the current

		// DC for printing
		m_bPrinting = pDC->m_bPrinting;
		m_hDC       = pDC->m_hDC;
		m_hAttribDC = pDC->m_hAttribDC;
	}
}

void CMemDC::mono()
{
	if (m_bitmap1.m_hObject == NULL)
	{
		CRect DP = m_rect;
		LPtoDP(&DP);
		DP.NormalizeRect();

		m_bitmap1.CreateBitmap(DP.Width(), DP.Height(), 1,1, NULL);
	}

	CBitmap *bm = SelectObject(&m_bitmap1);
	if (m_oldBitmap == NULL) m_oldBitmap = bm;

	// Fill background
	FillSolidRect(m_rect, C_RGB::mono_off);
}

void CMemDC::compatible()
{
	if (m_bitmap.m_hObject == NULL)
	{
		CRect DP = m_rect;
		LPtoDP(&DP);
		DP.NormalizeRect();

		m_bitmap.CreateCompatibleBitmap(this, DP.Width(), DP.Height());
	}

	CBitmap *bm = SelectObject(&m_bitmap);
	if (m_oldBitmap == NULL) m_oldBitmap = bm;

	// Fill background
	FillSolidRect(m_rect, GetBkColor());
}

CMemDC::~CMemDC()
{
	if (m_bMemDC)
	{
		//Swap back the original bitmap.
		if (m_oldBitmap != NULL) SelectObject(m_oldBitmap);
	}
	else
	{
		// All we need to do is replace the DC with an illegal
		// value, this keeps us from accidentally deleting the
		// handles associated with the CDC that was passed to
		// the constructor.

		m_hDC = m_hAttribDC = NULL;
	}
}

