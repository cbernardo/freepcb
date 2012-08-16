// DlgNetCombine.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgNetlist.h"
#include "DlgEditNet.h"
#include "DlgSetTraceWidths.h"
#include "DlgNetCombine.h"
#include "DlgChooseNetName.h"

extern CFreePcbApp theApp;

// columns for list
enum {
	COL_NAME = 0,
	COL_PINS,
	COL_WIDTH,
	COL_VIA_W,
	COL_HOLE_W
};

// sort types
enum {
	SORT_UP_NAME = 0,
	SORT_DOWN_NAME,
	SORT_UP_PINS,
	SORT_DOWN_PINS,
	SORT_UP_WIDTH,
	SORT_DOWN_WIDTH,
	SORT_UP_VIA_W,
	SORT_DOWN_VIA_W,
	SORT_UP_HOLE_W,
	SORT_DOWN_HOLE_W
};

// global so that it is available to Compare() for sorting list control items
CArray<CString> *gNames;

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//		
int CALLBACK CompareNetlistCombine( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	CString name1 = (*gNames)[lp1], name2 = (*gNames)[lp2];
	return name1.CompareNoCase(name2);
}

// CDlgNetCombine dialog

IMPLEMENT_DYNAMIC(CDlgNetCombine, CDialog)
CDlgNetCombine::CDlgNetCombine(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgNetCombine::IDD, pParent)
{
	m_names = NULL;
	m_pins = NULL;
}

CDlgNetCombine::~CDlgNetCombine()
{
}

void CDlgNetCombine::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_NET, m_list_ctrl);
	if( pDX->m_bSaveAndValidate )
	{
		POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
		int i_sel = m_list_ctrl.GetNextSelectedItem( pos );
		if (i_sel == -1)
			pDX->Fail();
		int i = m_list_ctrl.GetItemData( i_sel );
		m_new_name = (*m_names)[i];
	}
}


BEGIN_MESSAGE_MAP(CDlgNetCombine, CDialog)
END_MESSAGE_MAP()


void CDlgNetCombine::Initialize(CArray<CString> *names, CArray<int> *pins)
{
	m_names = gNames = names;
	m_pins = pins;
}

// CDlgNetCombine message handlers

BOOL CDlgNetCombine::OnInitDialog()
{
	CDialog::OnInitDialog();
	DrawListCtrl();
	return TRUE;  
}

// draw listview control and sort alphabetically by name
//
void CDlgNetCombine::DrawListCtrl()
{
	int nItem;
	CString str;
	DWORD old_style = m_list_ctrl.GetExtendedStyle(); 
	m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | old_style );
	m_list_ctrl.DeleteAllItems();
	CString colNames[5];
	for (int i=0; i<5; i++)
		colNames[i].LoadStringA(IDS_NetCombineCols+i);
	m_list_ctrl.InsertColumn( COL_NAME, colNames[0], LVCFMT_LEFT, 300 );
	m_list_ctrl.InsertColumn( COL_PINS, colNames[1], LVCFMT_LEFT, 60 );
	int iItem = 0;
	for( int i=0; i < m_names->GetSize(); i++ )
	{
			nItem = m_list_ctrl.InsertItem( iItem, "" );
			m_list_ctrl.SetItemData( iItem, (LPARAM)i );
			m_list_ctrl.SetItem( iItem, COL_NAME, LVIF_TEXT, (*m_names)[i], 0, 0, 0, 0 );
			str.Format( "%d", (*m_pins)[i] );
			m_list_ctrl.SetItem( iItem, COL_PINS, LVIF_TEXT, str, 0, 0, 0, 0 );
	}
	m_list_ctrl.SortItems( ::CompareNetlistCombine, 0 );
}

