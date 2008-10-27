// FootprintView.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "FootprintView.h"


// CFootprintView

IMPLEMENT_DYNCREATE(CFootprintView, CView)

CFootprintView::CFootprintView()
{
}

CFootprintView::~CFootprintView()
{
}


BEGIN_MESSAGE_MAP(CFootprintView, CView)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CFootprintView drawing

void CFootprintView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

//	CSize sizeTotal;
	// TODO: calculate the total size of this view
//	sizeTotal.cx = sizeTotal.cy = 100;
//	SetScrollSizes(MM_TEXT, sizeTotal);
}

void CFootprintView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}


// CFootprintView diagnostics

#ifdef _DEBUG
void CFootprintView::AssertValid() const
{
	CView::AssertValid();
}

void CFootprintView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG


// CFootprintView message handlers

BOOL CFootprintView::OnEraseBkgnd(CDC* pDC)
{
	CRect r;
	GetClientRect( &r );
	CPen black_pen( PS_SOLID, 1, RGB(0,0,0) );
	CPen white_pen( PS_SOLID, 1, RGB(255,255,255) );
	CBrush black_brush( RGB(0,0,0) );
	CPen * old_pen = pDC->SelectObject( &black_pen );
	CBrush * old_brush = pDC->SelectObject( &black_brush );
	pDC->Rectangle( r );
	pDC->SelectObject( &white_pen );
	pDC->Rectangle( r.left+100, r.top+100, r.left+200, r.top+200 );
	pDC->SelectObject( old_pen );
	pDC->SelectObject( old_brush );
	return FALSE;
}
