// DlgFindPart.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgFindPart.h"
#include "Part.h"


// CDlgFindPart dialog

IMPLEMENT_DYNAMIC(CDlgFindPart, CDialog)
CDlgFindPart::CDlgFindPart(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgFindPart::IDD, pParent)
{
}

CDlgFindPart::~CDlgFindPart()
{
}

void CDlgFindPart::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_ref_des);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming.  CPT2 Sort names in a CArray<CString> first.  It's really stupid, but I couldn't get the built-in
		// sorting of combo-boxes to work.  Damnable MSDN dox...
		CArray<CString> names;
		citer<cpart2> ip (&m_pl->parts);
		for (cpart2 *part = ip.First(); part; part = ip.Next())
			names.Add( part->ref_des );
		qsort(names.GetData(), names.GetSize(), sizeof(CString), (int (*)(const void*,const void*)) CompareNumeric);
		for (int i=0; i<names.GetSize(); i++)
			m_combo_ref_des.AddString( names[i] );
	}
	else
	{
		// outgoing
		m_combo_ref_des.GetWindowText( sel_ref_des );
	}
}


BEGIN_MESSAGE_MAP(CDlgFindPart, CDialog)
END_MESSAGE_MAP()


// CDlgFindPart message handlers

void CDlgFindPart::Initialize( cpartlist * pl )
{
	m_pl = pl;
}
