// DlgNetlist.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgNetlist.h"
#include "DlgEditNet.h"
#include "DlgSetTraceWidths.h"

extern CFreePcbApp theApp;

// columns for list
enum {
	COL_VIS = 0,
	COL_NAME,
	COL_PINS,
	COL_WIDTH,
	COL_VIA_W,
	COL_HOLE_W,
	COL_CLEARANCE
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
	SORT_DOWN_HOLE_W,
	SORT_UP_CLEARANCE,
	SORT_DOWN_CLEARANCE
};

// global so that it is available to Compare() for sorting list control items
static netlist_info nl;

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//
int CALLBACK CompareNetlist( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	int ret = 0;
	switch( type )
	{
		case SORT_UP_NAME:
		case SORT_DOWN_NAME:
			ret = (strcmp( ::nl[lp1].name, ::nl[lp2].name ));
			break;

		case SORT_UP_WIDTH:
		case SORT_DOWN_WIDTH:
			if( ::nl[lp1].width.m_seg_width > ::nl[lp2].width.m_seg_width )
				ret = 1;
			else if( ::nl[lp1].width.m_seg_width < ::nl[lp2].width.m_seg_width )
				ret = -1;
			break;

		case SORT_UP_VIA_W:
		case SORT_DOWN_VIA_W:
			if( ::nl[lp1].width.m_via_width > ::nl[lp2].width.m_via_width )
				ret = 1;
			else if( ::nl[lp1].width.m_via_width < ::nl[lp2].width.m_via_width )
				ret = -1;
			break;

		case SORT_UP_HOLE_W:
		case SORT_DOWN_HOLE_W:
			if( ::nl[lp1].width.m_via_hole > ::nl[lp2].width.m_via_hole )
				ret = 1;
			else if( ::nl[lp1].width.m_via_hole < ::nl[lp2].width.m_via_hole )
				ret = -1;
			break;

		case SORT_UP_PINS:
		case SORT_DOWN_PINS:
			if( ::nl[lp1].ref_des.GetSize() > ::nl[lp2].ref_des.GetSize() )
				ret = 1;
			else if( ::nl[lp1].ref_des.GetSize() < ::nl[lp2].ref_des.GetSize() )
				ret = -1;
			break;

		case SORT_UP_CLEARANCE:
		case SORT_DOWN_CLEARANCE:
			if( ::nl[lp1].clearance.m_ca_clearance > ::nl[lp2].clearance.m_ca_clearance )
				ret = 1;
			else if( ::nl[lp1].clearance.m_ca_clearance < ::nl[lp2].clearance.m_ca_clearance )
				ret = -1;
			break;
	}
	switch( type )
	{
		case SORT_DOWN_NAME:
		case SORT_DOWN_WIDTH:
		case SORT_DOWN_VIA_W:
		case SORT_DOWN_HOLE_W:
		case SORT_DOWN_PINS:
		case SORT_DOWN_CLEARANCE:
			ret = -ret;
			break;
	}

	return ret;
}

// CDlgNetlist dialog

IMPLEMENT_DYNAMIC(CDlgNetlist, CDialog)
CDlgNetlist::CDlgNetlist(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgNetlist::IDD, pParent)
{
	m_w = 0;
	m_v_w = 0;
	m_v_h_w = 0;
	m_nlist = 0;
}

CDlgNetlist::~CDlgNetlist()
{
}

void CDlgNetlist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_NET, m_list_ctrl);
	DDX_Control(pDX, IDC_BUTTON_VISIBLE, m_button_visible);
	DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_edit);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_delete);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add);
	DDX_Control(pDX, IDC_BUTTON_SELECT_ALL, m_button_select_all);
	DDX_Control(pDX, IDC_BUTTON_NL_WIDTH, m_button_nl_width);
	DDX_Control(pDX, IDOK, m_OK);
	DDX_Control(pDX, IDCANCEL, m_cancel);
	DDX_Control(pDX, IDC_BUTTON_DELETE2, m_button_delete_single);
	if( pDX->m_bSaveAndValidate )
	{
		// update ::nl with visibility data before leaving
		int n_local_nets = ::nl.GetSize();
		for( int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ )
		{
			int i = m_list_ctrl.GetItemData( iItem );
			::nl[i].visible = ListView_GetCheckState( m_list_ctrl, iItem );
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgNetlist, CDialog)
//	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_NET, OnLvnItemchangedListNet)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_NET, OnLvnColumnclickListNet)
	ON_BN_CLICKED(IDC_BUTTON_VISIBLE, OnBnClickedButtonVisible)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnBnClickedButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_DELETE2, OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_NL_WIDTH, OnBnClickedButtonNLWidth)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_NOPINS, OnBnClickedDeleteNetsWithNoPins)
	ON_NOTIFY(NM_CLICK, IDC_LIST_NET, OnNMClickListNet)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_NET, &CDlgNetlist::OnNMDblclkListNet)
END_MESSAGE_MAP()


void CDlgNetlist::Initialize( CNetList * nlist, CPartList * plist,
		CArray<int> * w, CArray<int> * v_w, CArray<int> * v_h_w )
{
	m_nlist = nlist;
	m_plist = plist;
	m_w = w;
	m_v_w = v_w;
	m_v_h_w = v_h_w;
}

// CDlgNetlist message handlers

BOOL CDlgNetlist::OnInitDialog()
{
	CDialog::OnInitDialog();

	// make copy of netlist data so that it can be edited but user can still cancel
	// use global netlist_info so it can be sorted in the list control
	m_nl = &::nl;
	m_nlist->ExportNetListInfo( &::nl );

	m_list_ctrl.InsertColumn( COL_VIS, "Vis", LVCFMT_LEFT, 25 );
	m_list_ctrl.InsertColumn( COL_NAME, "Name", LVCFMT_LEFT, 140 );
	m_list_ctrl.InsertColumn( COL_PINS, "Pins", LVCFMT_LEFT, 40 );
	m_list_ctrl.InsertColumn( COL_WIDTH, "Width", LVCFMT_LEFT, 40 );
	m_list_ctrl.InsertColumn( COL_VIA_W, "Via W", LVCFMT_LEFT, 40 );
	m_list_ctrl.InsertColumn( COL_HOLE_W, "Hole", LVCFMT_LEFT, 40 );
	m_list_ctrl.InsertColumn( COL_CLEARANCE, "Clearance", LVCFMT_LEFT, 70 );

	// initialize netlist control
	m_item_selected = -1;
	m_sort_type = 0;
	m_visible_state = 1;
	DrawListCtrl();

	// initialize buttons
	m_button_edit.EnableWindow(FALSE);
	m_button_delete_single.EnableWindow(FALSE);
	m_button_nl_width.EnableWindow(FALSE);
	m_button_delete.EnableWindow(FALSE);

	return TRUE;
}


// draw listview control and sort according to m_sort_type
//
void CDlgNetlist::DrawListCtrl()
{
	int nItem;
	CString str;
	DWORD old_style = m_list_ctrl.GetExtendedStyle();
	m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | LVS_EX_CHECKBOXES | old_style );
	m_list_ctrl.DeleteAllItems();

	int iItem = 0;
	for( int i=0; i<::nl.GetSize(); i++ )
	{
		if( ::nl[i].deleted == FALSE )
		{
			nItem = m_list_ctrl.InsertItem( iItem, "" );
			m_list_ctrl.SetItemData( iItem, (LPARAM)i );

			m_list_ctrl.SetItem( iItem, COL_NAME, LVIF_TEXT, ::nl[i].name, 0, 0, 0, 0 );
			str.Format( "%d", ::nl[i].ref_des.GetSize() );
			m_list_ctrl.SetItem( iItem, COL_PINS, LVIF_TEXT, str, 0, 0, 0, 0 );

			str = CInheritableInfo::GetItemText( ::nl[i].width.m_seg_width );
			m_list_ctrl.SetItem( iItem, COL_WIDTH, LVIF_TEXT, str, 0, 0, 0, 0 );

			str = CInheritableInfo::GetItemText( ::nl[i].width.m_via_width );
			m_list_ctrl.SetItem( iItem, COL_VIA_W, LVIF_TEXT, str, 0, 0, 0, 0 );

			str = CInheritableInfo::GetItemText( ::nl[i].width.m_via_hole );
			m_list_ctrl.SetItem( iItem, COL_HOLE_W, LVIF_TEXT, str, 0, 0, 0, 0 );

			str = CInheritableInfo::GetItemText( ::nl[i].clearance.m_ca_clearance );
			m_list_ctrl.SetItem( iItem, COL_CLEARANCE, LVIF_TEXT, str, 0, 0, 0, 0 );

			ListView_SetCheckState( m_list_ctrl, nItem, ::nl[i].visible );
		}
	}
	m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
}

void CDlgNetlist::OnLvnColumnclickListNet(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int column = pNMLV->iSubItem;
	if( column == COL_NAME )
	{
		if( m_sort_type == SORT_UP_NAME )
			m_sort_type = SORT_DOWN_NAME;
		else
			m_sort_type = SORT_UP_NAME;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_WIDTH )
	{
		if( m_sort_type == SORT_UP_WIDTH )
			m_sort_type = SORT_DOWN_WIDTH;
		else
			m_sort_type = SORT_UP_WIDTH;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_VIA_W )
	{
		if( m_sort_type == SORT_UP_VIA_W )
			m_sort_type = SORT_DOWN_VIA_W;
		else
			m_sort_type = SORT_UP_VIA_W;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_HOLE_W )
	{
		if( m_sort_type == SORT_UP_HOLE_W )
			m_sort_type = SORT_DOWN_HOLE_W;
		else
			m_sort_type = SORT_UP_HOLE_W;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_CLEARANCE )
	{
		if( m_sort_type == SORT_UP_CLEARANCE )
			m_sort_type = SORT_DOWN_CLEARANCE;
		else
			m_sort_type = SORT_UP_CLEARANCE;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_PINS )
	{
		if( m_sort_type == SORT_UP_PINS )
			m_sort_type = SORT_DOWN_PINS;
		else
			m_sort_type = SORT_UP_PINS;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	*pResult = 0;
}


void CDlgNetlist::OnBnClickedButtonVisible()
{
	for( int i=0; i<m_list_ctrl.GetItemCount(); i++ )
	{
		ListView_SetCheckState( m_list_ctrl, i, m_visible_state );
	}
	for( int i=0; i<::nl.GetSize(); i++ )
	{
		::nl[i].visible = m_visible_state;
	}
	m_visible_state =  1 - m_visible_state;
}

void CDlgNetlist::OnBnClickedButtonEdit()
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
		AfxMessageBox( "You have no net selected" );
	else if( n_sel > 1 )
		AfxMessageBox( "You have more than one net selected" );
	else
	{
		POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
		if( pos == NULL )
			ASSERT(0);

		int nItem = m_list_ctrl.GetNextSelectedItem(pos);
		int i = m_list_ctrl.GetItemData( nItem );

		// prepare and invoke dialog
		CFreePcbView * view = theApp.m_View;
		CFreePcbDoc * doc = theApp.m_Doc;
		CDlgEditNet dlg;
		dlg.Initialize( doc->m_nlist, &nl, i, m_plist, FALSE, ListView_GetCheckState( m_list_ctrl, nItem ),
						MIL, &(doc->m_w), &(doc->m_v_w), &(doc->m_v_h_w) );
		int ret = dlg.DoModal();
		if( ret == IDOK )
		{
			// implement edits into nl and update m_list_ctrl
			DrawListCtrl();
		}
	}
}

void CDlgNetlist::OnBnClickedButtonAdd()
{
	// prepare CDlgEditNet
	CFreePcbView * view = theApp.m_View;
	CFreePcbDoc * doc = theApp.m_Doc;
	CDlgEditNet dlg;
	dlg.Initialize( doc->m_nlist, &nl, -1, m_plist, TRUE, TRUE,
					MIL, &doc->m_w, &doc->m_v_w, &doc->m_v_h_w );
	// invoke dialog
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// net added, update m_list_ctrl
		DrawListCtrl();
	}
}

void CDlgNetlist::OnBnClickedOk()
{
	OnOK();
}

void CDlgNetlist::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void CDlgNetlist::OnBnClickedButtonSelectAll()
{
	for( int i=0; i<::nl.GetSize(); i++ )
		m_list_ctrl.SetItemState( i, LVIS_SELECTED, LVIS_SELECTED );
	OnNMClickListNet( NULL, NULL );
}

void CDlgNetlist::OnBnClickedButtonDelete()
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
	{
		AfxMessageBox( "You have no net(s) selected" );
	}
	else
	{
		// now delete them
		while( m_list_ctrl.GetSelectedCount() )
		{
			POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
			int i_sel = m_list_ctrl.GetNextSelectedItem( pos );
			int i = m_list_ctrl.GetItemData( i_sel );
			::nl[i].deleted = TRUE;
			m_list_ctrl.DeleteItem( i_sel );
		}
	}
}

void CDlgNetlist::OnBnClickedButtonNLWidth()
{
	CString str;
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
	{
		AfxMessageBox( "You have no net(s) selected" );
		return;
	}
	CFreePcbView * view = theApp.m_View;
	CFreePcbDoc * doc = theApp.m_Doc;
	CDlgSetTraceWidths dlg;
	dlg.m_w = &doc->m_w;
	dlg.m_v_w = &doc->m_v_w;
	dlg.m_v_h_w = &doc->m_v_h_w;
	dlg.m_width = CConnectionWidthInfo(CInheritableInfo::E_USE_PARENT, CInheritableInfo::E_USE_PARENT, CInheritableInfo::E_USE_PARENT);
	dlg.m_clearance.m_ca_clearance = CInheritableInfo::E_USE_PARENT;
	if( n_sel == 1 )
	{
		POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
		int iItem = m_list_ctrl.GetNextSelectedItem( pos );
		int i = m_list_ctrl.GetItemData( iItem );

		dlg.m_width = ::nl[i].width;
        dlg.m_clearance = ::nl[i].clearance;
	}
	else
	{
		// Assign the project clearance (in doc) as the parent
		dlg.m_clearance.SetParent(doc->m_def_size_attrib);
		dlg.m_width    .SetParent(doc->m_def_size_attrib);
	}

	dlg.m_width.Update();
	dlg.m_clearance.Update();

	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
		while( pos )
		{
			int iItem = m_list_ctrl.GetNextSelectedItem( pos );
			int i = m_list_ctrl.GetItemData( iItem );

			if( dlg.m_apply_trace )
			{
				::nl[i].width.m_seg_width = dlg.m_width.m_seg_width;
			}

			if( dlg.m_apply_via )
			{
				::nl[i].width.m_via_width = dlg.m_width.m_via_width;
				::nl[i].width.m_via_hole  = dlg.m_width.m_via_hole;
			}

			if( dlg.m_apply_clearance )
			{
				::nl[i].clearance = dlg.m_clearance;
			}

			::nl[i].apply_trace_width = dlg.m_apply_to_routed && dlg.m_apply_trace;
			::nl[i].apply_via_width   = dlg.m_apply_to_routed && dlg.m_apply_via;
    		::nl[i].apply_clearance   = dlg.m_apply_to_routed && dlg.m_apply_clearance;

			str = CInheritableInfo::GetItemText( ::nl[i].width.m_seg_width );
			m_list_ctrl.SetItem( iItem, COL_WIDTH, LVIF_TEXT, str, 0, 0, 0, 0 );

			str = CInheritableInfo::GetItemText( ::nl[i].width.m_via_width );
			m_list_ctrl.SetItem( iItem, COL_VIA_W, LVIF_TEXT, str, 0, 0, 0, 0 );

			str = CInheritableInfo::GetItemText( ::nl[i].width.m_via_hole );
			m_list_ctrl.SetItem( iItem, COL_HOLE_W, LVIF_TEXT, str, 0, 0, 0, 0 );

			str = CInheritableInfo::GetItemText( ::nl[i].clearance.m_ca_clearance );
			m_list_ctrl.SetItem( iItem, COL_CLEARANCE, LVIF_TEXT, str, 0, 0, 0, 0 );
		}
	}
}

void CDlgNetlist::OnBnClickedDeleteNetsWithNoPins()
{
	for( int iItem=m_list_ctrl.GetItemCount()-1; iItem>=0; iItem-- )
	{
		int i_net = m_list_ctrl.GetItemData( iItem );
		if( ::nl[i_net].ref_des.GetSize() == 0 )
		{
			::nl[i_net].deleted = TRUE;
			m_list_ctrl.DeleteItem( iItem );
		}
	}
}



void CDlgNetlist::OnNMClickListNet(NMHDR *pNMHDR, LRESULT *pResult)
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
	{
		m_button_edit.EnableWindow(FALSE);
		m_button_delete_single.EnableWindow(FALSE);
		m_button_nl_width.EnableWindow(FALSE);
		m_button_delete.EnableWindow(FALSE);
	}
	else if( n_sel == 1 )
	{
		m_button_edit.EnableWindow(TRUE);
		m_button_delete_single.EnableWindow(TRUE);
		m_button_nl_width.EnableWindow(FALSE);
		m_button_delete.EnableWindow(FALSE);
	}
	else
	{
		m_button_edit.EnableWindow(FALSE);
		m_button_delete_single.EnableWindow(FALSE);
		m_button_nl_width.EnableWindow(TRUE);
		m_button_delete.EnableWindow(TRUE);
	}
	if( pResult )
		*pResult = 0;
}


void CDlgNetlist::OnNMDblclkListNet(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnBnClickedButtonEdit();

	*pResult = 0;
}
