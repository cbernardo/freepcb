#ifndef _DRAWINGJOB_H /* [ */
#define _DRAWINGJOB_H

#include "afxwin.h"
#include "ids.h"
#include "layers.h"
#include "LinkList.h"

class CPolyLine;

class CDL_job: public CDLinkList
{
protected:
	CDisplayList * m_dlist;

	CDLinkList m_LIST_DLE;

public:
	CDL_job(CDisplayList * dlist) : m_dlist(dlist) {}
	virtual ~CDL_job();

	void Add(dl_element *pDLE);

public:
	struct SHitInfo
	{
		int layer;
		id  ID;
		void *ptr;
		int priority;
	};

	// returns # hits
	int TestForHit( CPoint const &point, SHitInfo hitInfo[], int max_hits ) const;

	virtual void Draw(CDrawInfo &di) const {}
};

class CDL_job_copper_area : public CDL_job
{
	void ScratchClearances(CDrawInfo &di, int layer, CRect const &area_bounds, void * area_net) const;
	CPolyLine * my_poly;

public:
	CDL_job_copper_area(CDisplayList * dlist, CPolyLine * poly) : 
	  CDL_job(dlist),
	  my_poly(poly)
	{
	}

	virtual void Draw(CDrawInfo &di) const;
};

class CDL_job_traces : public CDL_job
{
	friend CDL_job_copper_area;

public:
	CDL_job_traces(CDisplayList * dlist) : CDL_job(dlist) {}

	virtual void Draw(CDrawInfo &di) const;

	void UpdateLineWidths( int width, int layer );
};

#endif /* !_DRAWINGJOB_H ] */
