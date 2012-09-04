// DlgGroupPaste.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgGroupPaste.h"

// columns for list
enum {
	COL_VIS = 0,
	COL_NAME,
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

// global so that it is available to ListCompare() for sorting list control items
netlist_info gnl;

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//		
int CALLBACK ListCompare( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	int ret = 0;
	switch( type )
	{
		case SORT_UP_NAME:
		case SORT_DOWN_NAME:
			ret = (strcmp( ::gnl[lp1].name, ::gnl[lp2].name ));
			break;

		// CPT2 kind of dubious that this is worth all the trouble :{ ...
		case SORT_UP_WIDTH:
		case SORT_DOWN_WIDTH:
			if( ::gnl[lp1].w > ::gnl[lp2].w )
				ret = 1;
			else if( ::gnl[lp1].w < ::gnl[lp2].w )
				ret = -1;
			break;

		case SORT_UP_VIA_W:
		case SORT_DOWN_VIA_W:
			if( ::gnl[lp1].v_w > ::gnl[lp2].v_w )
				ret = 1;
			else if( ::gnl[lp1].v_w < ::gnl[lp2].v_w )
				ret = -1;
			break;

		case SORT_UP_HOLE_W:
		case SORT_DOWN_HOLE_W:
			if( ::gnl[lp1].v_h_w > ::gnl[lp2].v_h_w )
				ret = 1;
			else if( ::gnl[lp1].v_h_w < ::gnl[lp2].v_h_w )
				ret = -1;
			break;

		case SORT_UP_PINS:
		case SORT_DOWN_PINS:
			if( ::gnl[lp1].ref_des.GetSize() > ::gnl[lp2].ref_des.GetSize() )
				ret = 1;
			else if( ::gnl[lp1].ref_des.GetSize() < ::gnl[lp2].ref_des.GetSize() )
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
			ret = -ret;
			break;
	}

	return ret;
}

// CDlgGroupPaste dialog

IMPLEMENT_DYNAMIC(CDlgGroupPaste, CDialog)
CDlgGroupPaste::CDlgGroupPaste(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgGroupPaste::IDD, pParent)
{
}

CDlgGroupPaste::~CDlgGroupPaste()
{
}

void CDlgGroupPaste::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_USE_GROUP_REF, m_radio_use_group_ref);
	DDX_Control(pDX, IDC_RADIO_USE_NEXT_REF, m_radio_use_next_ref);
	DDX_Control(pDX, IDC_RADIO_ADD_REF_OFFSET, m_radio_add_ref_offset);
	DDX_Control(pDX, IDC_EDIT_REF_OFFSET, m_edit_offset);
	DDX_Control(pDX, IDC_RADIO_USE_GROUP_NETS, m_radio_use_group_nets);
	DDX_Control(pDX, IDC_RADIO_USE_SELECTED_NETS, m_radio_use_selected_nets);
	DDX_Control(pDX, IDC_LIST_SELECT_GROUP_NETS, m_list_ctrl);
	DDX_Control(pDX, IDC_RADIO_USE_SUFFIX, m_radio_use_suffix);
	DDX_Control(pDX, IDC_RADIO_MAKE_NEW_NAMES, m_radio_make_new_names);
	DDX_Control(pDX, IDC_RADIO_DRAG, m_radio_drag);
	DDX_Control(pDX, IDC_RADIO_OFFSET, m_radio_offset);
	DDX_Control(pDX, IDC_EDIT_GROUP_X, m_edit_x);
	DDX_Control(pDX, IDC_EDIT_GROUP_Y, m_edit_y);
	DDX_Control(pDX, IDC_COMBO_GROUP_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_CHECK_IGNORE_EMPTY_NETS, m_check_ignore_empty_nets);
	DDX_Control(pDX, IDC_CHECK_MULTIPLE, m_check_multiple);
	DDX_Control(pDX, IDC_EDIT_NUM_COPIES, m_edit_num_copies);
	DDX_Text( pDX, IDC_EDIT_GROUP_X, m_dx );
	DDX_Text( pDX, IDC_EDIT_GROUP_Y, m_dy );
	DDX_Text( pDX, IDC_EDIT_REF_OFFSET, m_ref_offset );
	DDX_Text( pDX, IDC_EDIT_NUM_COPIES, m_num_copies );
	DDV_MinMaxUInt(pDX, m_num_copies, 1, 99);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_sort_type = 0;
		m_combo_units.AddString( "MM" );
		m_combo_units.AddString( "MIL" );
		// now set up listview control
		int nItem;
		LVITEM lvitem;
		CString str;
		DWORD old_style = m_list_ctrl.GetExtendedStyle();
		m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | LVS_EX_CHECKBOXES | old_style );
		CString colNames[6];
		for (int i=0; i<6; i++)
			colNames[i].LoadStringA(IDS_GroupPasteCols+i);
		m_list_ctrl.InsertColumn( COL_VIS, colNames[0], LVCFMT_LEFT, 30 ); 
		m_list_ctrl.InsertColumn( COL_NAME, colNames[1], LVCFMT_LEFT, 140 );
		m_list_ctrl.InsertColumn( COL_PINS, colNames[2], LVCFMT_LEFT, 40 );
		m_list_ctrl.InsertColumn( COL_WIDTH, colNames[3], LVCFMT_LEFT, 40 );
		m_list_ctrl.InsertColumn( COL_VIA_W, colNames[4], LVCFMT_LEFT, 40 );   
		m_list_ctrl.InsertColumn( COL_HOLE_W, colNames[5], LVCFMT_LEFT, 40 );
		for( int i=0; i<gnl.GetSize(); i++ )
		{
			lvitem.mask = LVIF_TEXT | LVIF_PARAM;
			lvitem.pszText = "";
			lvitem.lParam = i;
			nItem = m_list_ctrl.InsertItem( i, "" );
			m_list_ctrl.SetItemData( i, (LPARAM)i );
			m_list_ctrl.SetItem( i, COL_NAME, LVIF_TEXT, ::gnl[i].name, 0, 0, 0, 0 );
			str.Format( "%d", ::gnl[i].ref_des.GetSize() );
			m_list_ctrl.SetItem( i, COL_PINS, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", ::gnl[i].w/NM_PER_MIL );
			m_list_ctrl.SetItem( i, COL_WIDTH, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", ::gnl[i].v_w/NM_PER_MIL );
			m_list_ctrl.SetItem( i, COL_VIA_W, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", ::gnl[i].v_h_w/NM_PER_MIL );
			m_list_ctrl.SetItem( i, COL_HOLE_W, LVIF_TEXT, str, 0, 0, 0, 0 );
			ListView_SetCheckState( m_list_ctrl, nItem, 0 );
		}
		// Attempt to restore all the checks and text values from last time.  If there was no last time,
		// make some default selections. 
		if (!RestoreState())
		{
			m_radio_use_next_ref.SetCheck(1); 
			m_radio_use_selected_nets.SetCheck(1);
			m_radio_use_suffix.SetCheck(1);
			m_radio_drag.SetCheck(1);
			m_combo_units.SelectString( 0, "MIL" );
			m_edit_num_copies.EnableWindow(false);
		}
		SetFields();
	}

	else
	{
		// outgoing
		SetFields();
		PreserveState();
		// It's convenient to pass back a single flag variable to the caller:
		m_flags = 0;
		if (m_ref_option==0)
			m_flags |= PASTE_PARTS_RETAIN;
		else if (m_ref_option==1)
			m_flags |= PASTE_PARTS_NEXT_AVAIL;
		else
			m_flags |= PASTE_PARTS_OFFSET;
		if (m_pin_net_option == 1)
			m_flags |= PASTE_NETS_IGNORE_TRACELESS;
		if (m_net_name_option == 0)
			m_flags |= PASTE_NETS_MERGE_ALL;
		else
			m_flags |= PASTE_NETS_MERGE_SOME;
		if (m_net_rename_option == 0)
			m_flags |= PASTE_NETS_RENAME_G;
		if (m_position_option == 0)
			m_flags |= PASTE_DRAG;
		// Adjust dx/dy according to the units:
		CString str;
		m_combo_units.GetWindowText( str );
		if( str == "MIL" )
		{
			m_dx *= NM_PER_MIL;
			m_dy *= NM_PER_MIL;
		}
		else
		{
			m_dx *= 1000000;
			m_dy *= 1000000;
		}
		// get info about which nets are selected into the clipboard nets' utility fields.
		for( int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ )
		{
			int i = m_list_ctrl.GetItemData( iItem ); 
			CString *net_name = &::gnl[i].name;
			CNet *grp_net = m_grp_nlist->GetNetPtrByName( net_name );
			ASSERT( grp_net != NULL );
			grp_net->utility = ListView_GetCheckState( m_list_ctrl, iItem );
		}
		if (!m_check_multiple.GetCheck())
			m_num_copies = 1;							// Even if the text-box says something else...
		::gnl.SetSize(0);
	}
}



BEGIN_MESSAGE_MAP(CDlgGroupPaste, CDialog)
	ON_BN_CLICKED(IDC_RADIO_USE_GROUP_REF, OnBnClickedRadioUseGroupRef)
	ON_BN_CLICKED(IDC_RADIO_USE_NEXT_REF, OnBnClickedRadioUseNextRef)
	ON_BN_CLICKED(IDC_RADIO_ADD_REF_OFFSET, OnBnClickedRadioAddRefOffset)
	ON_BN_CLICKED(IDC_RADIO_USE_GROUP_NETS, OnBnClickedRadioUseGroupNets)
	ON_BN_CLICKED(IDC_RADIO_USE_SELECTED_NETS, OnBnClickedRadioUseSelectedNets)
	ON_BN_CLICKED(IDC_RADIO_USE_SUFFIX, OnBnClickedRadioUseSuffix)
	ON_BN_CLICKED(IDC_RADIO_MAKE_NEW_NAMES, OnBnClickedRadioMakeNewNames)
	ON_BN_CLICKED(IDC_RADIO_DRAG, OnBnClickedRadioDrag)
	ON_BN_CLICKED(IDC_RADIO_OFFSET, OnBnClickedRadioOffset)
	ON_BN_CLICKED(IDC_CHECK_MULTIPLE, OnBnClickedCheckMultiple)
	ON_CBN_SELCHANGE(IDC_COMBO_GROUP_UNITS, OnCbnSelchangeComboGroupUnits)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_SELECT_GROUP_NETS, OnLvnColumnClickListSelectGroupNets)
END_MESSAGE_MAP()

void CDlgGroupPaste::Initialize( CNetList *grp_nlist )
{
	m_grp_nlist = grp_nlist;
	grp_nlist->nets.SetUtility(0);								// dlg will set utility bits for the nets in this list, in some cases
	m_ref_offset = 0;
	m_dx = 0;
	m_dy = 0;
	m_num_copies = 1;
	grp_nlist->ExportNetListInfo( &gnl );
}

bool g_bPrevious = false;
int g_ref_option, g_net_name_option, g_net_rename_option, g_pin_net_option, g_position_option;
bool g_bMultiple;
int g_combo_units;
CString g_x, g_y, g_offset, g_num_copies;
CMapStringToPtr g_sel_nets;

void CDlgGroupPaste::PreserveState()
{
	// Preserve the state of the dlg in global variables for next time it opens.  Assumes that SetFields() has run.
	g_bPrevious = true;
	g_ref_option = m_ref_option;
	g_net_name_option = m_net_name_option;
	g_net_rename_option = m_net_rename_option;
	g_pin_net_option = m_pin_net_option;
	g_position_option = m_position_option;
	g_bMultiple = m_check_multiple.GetCheck();
	g_combo_units = m_combo_units.GetCurSel();
	m_edit_x.GetWindowTextA(g_x);
	m_edit_y.GetWindowTextA(g_y);
	m_edit_offset.GetWindowTextA(g_offset);
	m_edit_num_copies.GetWindowTextA(g_num_copies);
	// get info about which nets are selected, and save their names in the map g_sel_nets.
	g_sel_nets.RemoveAll();
	for( int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ )
	{
		int i = m_list_ctrl.GetItemData( iItem ); 
		CString net_name = ::gnl[i].name;
		bool bState = ListView_GetCheckState( m_list_ctrl, iItem );
		if (bState)
			g_sel_nets.SetAt(net_name, (void*) 1);
	}	
}

bool CDlgGroupPaste::RestoreState()
{
	// Restore the state of the dlg to what it was on the last exit.  Invoked from within incoming DoDataExchange().  Assumes basic setup (e.g.
	// of m_combo_units and m_list_view) has occurred, but no checking.
	// Returns false if there is no prior state to restore.
	if (!g_bPrevious) return false;
	if (g_ref_option==0)
		m_radio_use_group_ref.SetCheck(true);
	else if (g_ref_option==1)
		m_radio_use_next_ref.SetCheck(true);
	else
		m_radio_add_ref_offset.SetCheck(true);
	if (g_net_name_option==0)
		m_radio_use_group_nets.SetCheck(true);
	else
		m_radio_use_selected_nets.SetCheck(true);
	if (g_net_rename_option==0)
		m_radio_use_suffix.SetCheck(true);
	else
		m_radio_make_new_names.SetCheck(true);
	if (g_pin_net_option)
		m_check_ignore_empty_nets.SetCheck(true);
	if (g_position_option)
		m_radio_offset.SetCheck(true);
	else
		m_radio_drag.SetCheck(true);
	if (g_bMultiple)
		m_check_multiple.SetCheck(true);
	m_combo_units.SetCurSel( g_combo_units );
	m_edit_x.SetWindowTextA(g_x);
	m_edit_y.SetWindowTextA(g_y);
	m_edit_offset.SetWindowTextA(g_offset);
	m_edit_num_copies.SetWindowTextA(g_num_copies);
	// Set checks in listview according to which net names are found in g_sel_nets.
	for( int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ )
	{
		int i = m_list_ctrl.GetItemData( iItem ); 
		CString net_name = ::gnl[i].name;
		if (g_sel_nets[net_name]) 
			ListView_SetCheckState( m_list_ctrl, iItem, true );
	}	
	return true;
}

void CDlgGroupPaste::SetFields()
{
	// set values and enable/disable items based on selections
	if (m_check_multiple.GetCheck())
		m_radio_drag.SetCheck(false),
		m_radio_drag.EnableWindow(false),
		m_radio_offset.SetCheck(true),
		m_edit_num_copies.EnableWindow(true);
	else
		m_radio_drag.EnableWindow(true),
		m_edit_num_copies.EnableWindow(false);

	if( m_radio_use_group_ref.GetCheck() )
	{
		m_ref_option = 0;
		m_edit_offset.EnableWindow(0);
	}
	else if( m_radio_use_next_ref.GetCheck() )
	{
		m_ref_option = 1;
		m_edit_offset.EnableWindow(0);
	}
	else if( m_radio_add_ref_offset.GetCheck() )
	{
		m_ref_option = 2;
		m_edit_offset.EnableWindow(1);
	}
	if( m_radio_use_group_nets.GetCheck() )
	{
		m_net_name_option = 0;
		m_list_ctrl.EnableWindow(0);
		m_radio_use_suffix.EnableWindow(0);
		m_radio_make_new_names.EnableWindow(0);
	}
	else if( m_radio_use_selected_nets.GetCheck() )
	{
		m_net_name_option = 1;
		m_list_ctrl.EnableWindow(1);
		m_radio_use_suffix.EnableWindow(1);
		m_radio_make_new_names.EnableWindow(1);
	}
	if( m_radio_drag.GetCheck() )
	{
		m_position_option = 0;
		m_edit_x.EnableWindow(0);
		m_edit_y.EnableWindow(0);
		m_edit_num_copies.EnableWindow(false);
		m_combo_units.EnableWindow(0);
	}
	else if( m_radio_offset.GetCheck() )
	{
		m_position_option = 1;
		m_edit_x.EnableWindow(1);
		m_edit_y.EnableWindow(1);
		m_combo_units.EnableWindow(1);
	}
	if( m_radio_use_suffix.GetCheck() )
		m_net_rename_option = 0;
	else if( m_radio_make_new_names.GetCheck() )
		m_net_rename_option = 1;
	if( m_check_ignore_empty_nets.GetCheck() )
		m_pin_net_option = 1;
	else
		m_pin_net_option = 0;
}

// CDlgGroupPaste message handlers

void CDlgGroupPaste::OnBnClickedRadioUseGroupRef()
	{ SetFields(); }

void CDlgGroupPaste::OnBnClickedRadioUseNextRef()
	{ SetFields(); }

void CDlgGroupPaste::OnBnClickedRadioAddRefOffset()
	{ SetFields(); }

void CDlgGroupPaste::OnBnClickedRadioUseGroupNets()
	{ SetFields(); }

void CDlgGroupPaste::OnBnClickedRadioUseSelectedNets()
	{ SetFields(); }

void CDlgGroupPaste::OnBnClickedRadioUseSuffix()
	{ SetFields(); }

void CDlgGroupPaste::OnBnClickedRadioMakeNewNames()
	{ SetFields(); }

void CDlgGroupPaste::OnBnClickedRadioDrag()
	{ SetFields(); }

void CDlgGroupPaste::OnBnClickedRadioOffset()
	{ SetFields(); }

void CDlgGroupPaste::OnCbnSelchangeComboGroupUnits()
	{ }

void CDlgGroupPaste::OnBnClickedCheckMultiple()
	{ SetFields(); }

void CDlgGroupPaste::OnLvnColumnClickListSelectGroupNets(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int column = pNMLV->iSubItem;
	if( column == COL_NAME )
	{
		if( m_sort_type == SORT_UP_NAME )
			m_sort_type = SORT_DOWN_NAME;
		else
			m_sort_type = SORT_UP_NAME;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	else if( column == COL_WIDTH )
	{
		if( m_sort_type == SORT_UP_WIDTH )
			m_sort_type = SORT_DOWN_WIDTH;
		else
			m_sort_type = SORT_UP_WIDTH;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	else if( column == COL_VIA_W )
	{
		if( m_sort_type == SORT_UP_VIA_W )
			m_sort_type = SORT_DOWN_VIA_W;
		else
			m_sort_type = SORT_UP_VIA_W;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	else if( column == COL_HOLE_W )
	{
		if( m_sort_type == SORT_UP_HOLE_W )
			m_sort_type = SORT_DOWN_HOLE_W;
		else
			m_sort_type = SORT_UP_HOLE_W;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	else if( column == COL_PINS )
	{
		if( m_sort_type == SORT_UP_PINS )
			m_sort_type = SORT_DOWN_PINS;
		else
			m_sort_type = SORT_UP_PINS;
		m_list_ctrl.SortItems( ::ListCompare, m_sort_type );
	}
	*pResult = 0;
}

