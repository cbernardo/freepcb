#pragma once



// CFootprintView view

class CFootprintView : public CView
{
	DECLARE_DYNCREATE(CFootprintView)

public:
	CFootprintView();           // protected constructor used by dynamic creation
	virtual ~CFootprintView();

public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();     // first time after construct

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


