// DlgAddPart.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddPart.h"
#include "resource.h"
#include "PathDialog.h"


// save expanded state of local cache
BOOL gLocalCacheExpanded = FALSE;
CArray<CShape*> gTempCachePtrs;

// global for last ref des
CString last_ref_des = "";
CString last_footprint = "";

// CDlgAddPart dialog

IMPLEMENT_DYNAMIC(CDlgAddPart, CDialog)
CDlgAddPart::CDlgAddPart(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddPart::IDD, pParent)
{
	m_cache_shapes = NULL;
	m_units = MIL;
	m_shape = NULL;
}

CDlgAddPart::~CDlgAddPart()
{
}

void CDlgAddPart::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PART_LIB_TREE, part_tree);
	DDX_Control(pDX, IDC_FOOTPRINT, m_edit_footprint);
	DDX_Control(pDX, IDC_RADIO_DRAG, m_radio_drag);
	DDX_Control(pDX, IDC_RADIO_SET, m_radio_set);
	DDX_Control(pDX, IDC_RADIO_OFFBOARD, m_radio_offboard);
	DDX_Control(pDX, IDC_X, m_edit_x);
	DDX_Control(pDX, IDC_Y, m_edit_y);
	DDX_Control(pDX, IDC_LIST_SIDE, m_list_side);
	DDX_Control(pDX, IDC_PART_REF, m_edit_ref_des);
	DDX_Control(pDX, IDC_PART_PACKAGE, m_edit_package);
	DDX_Control(pDX, IDC_PREVIEW, m_preview);
	DDX_Control(pDX, IDC_COMBO_ADD_PART_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_EDIT_ADD_AUTHOR, m_edit_author);
	DDX_Control(pDX, IDC_EDIT_ADD_SOURCE, m_edit_source);
	DDX_Control(pDX, IDC_EDIT_ADD_DESC, m_edit_desc);
	DDX_Control(pDX, IDC_BUTTON_ADD_PART_BROWSE, m_button_browse);
	DDX_Control(pDX, IDC_EDIT_ADD_PART_LIB, m_edit_lib);
	DDX_Control(pDX, IDC_EDIT_VALUE, m_edit_value);
	DDX_Control(pDX, IDC_CHECK1, m_check_value_visible);
	DDX_Control(pDX, IDC_CHECK2, m_check_ref_visible);
	DDX_Control(pDX, IDC_COMBO2, m_combo_angle);
	DDX_Control(pDX, IDC_BUTTON_COMPARE_WITH_NETLIST, m_button_compare_with_netlist);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		m_edit_lib.SetWindowText( *m_folder->GetFullPath() );
		if (!m_doc->m_bSyncFile || m_multiple)
			m_button_compare_with_netlist.ShowWindow(SW_HIDE);
		return;
	}

	// outgoing
	// test for valid ref des
	CString ref_des_str;
	m_edit_ref_des.GetWindowText( ref_des_str );
	ref_des_str.Trim();
	if( !m_multiple )
	{
		CString ird ((LPCSTR) IDS_IllegalReferenceDesignator);
		if( ref_des_str == "" )
		{
			AfxMessageBox( ird );
			pDX->PrepareEditCtrl( IDC_PART_REF );
			pDX->Fail();
		}
		if( ref_des_str.FindOneOf( ". " ) != -1 )
		{
			CString mess = ird + " " + ref_des_str;
			AfxMessageBox( mess );
			pDX->PrepareEditCtrl( IDC_PART_REF );
			pDX->Fail();
		}
		for( int i=0; i<m_pl->GetSize(); i++ )
		{
			if( (*m_pl)[i].deleted )
				continue;
			if( m_new_part == FALSE && i == m_ip )
				continue;
			if( ref_des_str == (*m_pl)[i].ref_des )
			{
				CString mess, duplicateRef ((LPCSTR) IDS_DuplicateReference);
				mess.Format( duplicateRef, ref_des_str );
				AfxMessageBox( mess );
				pDX->PrepareEditCtrl( IDC_PART_REF );
				pDX->Fail();
			}
		}
	}

	//test for valid value string
	CString value_str;
	m_edit_value.GetWindowText( value_str );
	if( value_str.Find( "@" ) != -1 )
	{
		CString mess ((LPCSTR) IDS_ValueCannotContainAt);
		AfxMessageBox( mess );
		pDX->PrepareEditCtrl( IDC_EDIT_VALUE );
		pDX->Fail();
	}

	// CPT2. Determine which footprint user wants.  The old code went ahead and loaded fp's from libraries and made the 
	//   necessary changes to the cache, but I think it's more appropriate if that process occurs later, within ImportPartListInfo(). 
	//   In particular, consider the case where user calls this dlg from DlgPartList, then later hits the DlgPartList cancel button...  
	//   We don't want this dlg making any changes that user regrets.
	if( !m_multiple || (m_multiple_mask & MSK_FOOTPRINT) )
	{
		// We must take into account both the text in m_edit_footprint and the contents of m_shape, which may have been set when user clicked
		// in the tree-view.
		CString foot_str;
		m_edit_footprint.GetWindowText( foot_str );
		if (!m_shape || foot_str != m_shape->m_name)
			// Look for the shape named "foot_str" within the cache
			m_shape = m_cache_shapes->GetShapeByName( &foot_str ),
			m_shape_file_name = "";
		if (!m_shape)
		{
			CString str;
			if (foot_str=="")
				str.LoadStringA(IDS_ErrorNoFootprintSelected);
			else
				str.LoadStringA(IDS_ErrorCouldntLocateFootprintName);
			AfxMessageBox( str );
			pDX->Fail();
		}
	}

	// All checks are complete --- prepare to return.  1st set some output variables, and create a new part-info struct if necessary.
	last_ref_des = ref_des_str;
	if (m_shape)
		last_footprint = m_shape->m_name;
	if( m_new_part )
	{
		m_ip = m_pl->GetSize();
		m_pl->SetSize( m_ip + 1 );
	}
	ASSERT( m_ip>=0 );
	part_info *pi = &(*m_pl)[m_ip];

	// Put data into part-info struct.
	if (!m_multiple || (m_multiple_mask & MSK_FOOTPRINT))
	{
		if (m_new_part || pi->shape!=m_shape || m_multiple)
			// Reset ref and value dimensions based on the new shape's defaults (I guess...)
			pi->ref_size = m_shape->m_ref->m_font_size,
			pi->ref_width = m_shape->m_ref->m_stroke_width,
			pi->value_size = m_shape->m_value->m_font_size,
			pi->value_width = m_shape->m_value->m_stroke_width;
		pi->shape = m_shape;
		pi->shape_name = m_shape->m_name;
		pi->shape_file_name = m_shape_file_name;
	}
	if( !m_multiple || (m_multiple_mask & MSK_PACKAGE) )
		// update package
		m_edit_package.GetWindowText( pi->package );
	if( !m_multiple || (m_multiple_mask & MSK_VALUE) )
	{
		// update value
		m_edit_value.GetWindowText( pi->value );
		if( !m_multiple )
			pi->value_vis = m_check_value_visible.GetCheck();
	}
	if (!m_multiple)
	{
		// update other fields
		GetFields();
		pi->ref_vis = m_check_ref_visible.GetCheck();
		pi->ref_des = ref_des_str;
		pi->x = m_x;
		pi->y = m_y;
		int side = m_list_side.GetCurSel();
		pi->side = side;
		int cent_angle = 0;
		if( pi->shape )
			cent_angle = pi->shape->m_centroid->m_angle;
		pi->angle = ::GetPartAngleForReportedAngle( m_combo_angle.GetCurSel()*90, 
			cent_angle, side );
		pi->deleted = FALSE;
		if( m_radio_offboard.GetCheck() )
			pi->bOffBoard = TRUE;
		else
			pi->bOffBoard = FALSE;
	}
}


BEGIN_MESSAGE_MAP(CDlgAddPart, CDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_PART_LIB_TREE, OnTvnSelchangedPartLibTree)
	ON_BN_CLICKED(IDC_RADIO_DRAG, OnBnClickedRadioDrag)
	ON_BN_CLICKED(IDC_RADIO_SET, OnBnClickedRadioSet)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBO_ADD_PART_UNITS, OnCbnSelchangeComboAddPartUnits)
	ON_BN_CLICKED(IDC_BUTTON_ADD_PART_BROWSE, OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_RADIO_OFFBOARD, OnBnClickedRadioOffBoard)
	ON_BN_CLICKED(IDC_BUTTON_COMPARE_WITH_NETLIST, &CDlgAddPart::OnBnClickedButtonCompareWithNetlist)
END_MESSAGE_MAP()


//**************** public methods ********************

// initialize dialog
//	
void CDlgAddPart::Initialize( partlist_info * pli, 
							 int i, 
							 BOOL standalone,
							 BOOL new_part,
							 BOOL multiple,
							 int multiple_mask,
							 CFreePcbDoc *doc,
							 int units )
{
	m_pl = pli;
	m_ip = i;
	m_shape = i>=0? (*pli)[i].shape: NULL;
	m_standalone = standalone;
	m_new_part = new_part;
	m_multiple = multiple;
	m_multiple_mask = multiple_mask;
	m_doc = doc;
	m_cache_shapes = doc->m_slist;
	m_footlibfoldermap = &doc->m_footlibfoldermap;
	m_units = units;  
	m_dlg_log = doc->m_dlg_log;
	CString * last_folder_path = m_footlibfoldermap->GetLastFolder();
	m_folder = m_footlibfoldermap->GetFolder( last_folder_path, m_dlg_log );
	m_in_cache = FALSE;
}

//***************** message handlers *******************

BOOL CDlgAddPart::OnInitDialog()
{
	CString str;
	LPCSTR p;

	CDialog::OnInitDialog();
	CTreeCtrl * pCtrl = &part_tree;
	int i_exp = 0;

	InitPartLibTree();

	// list control for angle
	m_combo_angle.InsertString( 0, "0" );
	m_combo_angle.InsertString( 1, "90" );
	m_combo_angle.InsertString( 2, "180" );
	m_combo_angle.InsertString( 3, "270" );
	CString top ((LPCSTR) IDS_Top), bottom((LPCSTR) IDS_Bottom);
	m_list_side.InsertString( 0, top );
	m_list_side.InsertString( 1, bottom );

	// set up for new part or edit existing part
	if( m_new_part )
	{
		m_edit_ref_des.SetWindowText( "" );
		m_edit_footprint.SetWindowText( "" );
		if( last_ref_des != "" )
		{
			int num_pos = -1;
			for( int i=last_ref_des.GetLength()-1; i>=0; i-- )
			{
				if( last_ref_des[i] > '9' || last_ref_des[i] < '0' )
				{
					num_pos = i+1;
					break;
				}
			}
			if( num_pos > 0 && num_pos < last_ref_des.GetLength() )
			{
				CString num_part = last_ref_des.Right( last_ref_des.GetLength()-num_pos );
				int num = atoi( num_part );
				if( num > 0 )
				{
					BOOL done = FALSE;
					while( !done )
					{
						CString ref_des;
						ref_des.Format( "%s%d", last_ref_des.Left(num_pos), num+1 );
						done = TRUE;
						for( int i=0; i<m_pl->GetSize(); i++ )
						{
							if( m_pl->GetAt(i).ref_des == ref_des )
							{
								num++;
								done = FALSE;
								break;
							}
						}
						if( done )
						{
							m_edit_ref_des.SetWindowText( ref_des );
							m_edit_footprint.SetWindowText( last_footprint );
						}
					}
				}
			}
		}
		m_edit_package.SetWindowText( "" );
		m_units = m_units_initial = MIL;
		m_combo_units.SetCurSel(0);
		m_x = m_x_initial = 0;
		m_y = m_y_initial = 0;
		m_edit_x.SetWindowText( "0" );
		m_edit_y.SetWindowText( "0" );
		m_combo_angle.SetCurSel( 0 );
		m_list_side.SetCurSel( 0 );
		if( m_standalone )
		{
			m_radio_drag.SetCheck( 1 );
			m_radio_offboard.SetCheck( 0 );
			m_radio_set.SetCheck( 0 );
			m_drag_flag = TRUE;
			m_combo_units.EnableWindow( FALSE );
			m_edit_x.EnableWindow( FALSE );
			m_edit_y.EnableWindow( FALSE );
			m_combo_angle.EnableWindow( FALSE );
			m_list_side.EnableWindow( FALSE );
		}
		else if( !m_standalone )
		{
			m_radio_drag.SetCheck( 0 );
			m_radio_offboard.SetCheck( 1 );
			m_radio_set.SetCheck( 0 );
			m_radio_drag.EnableWindow( 0 );
			m_drag_flag = FALSE;
			m_combo_units.EnableWindow( FALSE );
			m_edit_x.EnableWindow( FALSE );
			m_edit_y.EnableWindow( FALSE );
			m_combo_angle.EnableWindow( FALSE );
			m_list_side.EnableWindow( FALSE );
		}
		m_check_value_visible.SetCheck( FALSE ); // CPT
		m_check_ref_visible.SetCheck( TRUE ); // CPT
	}
	else if( m_multiple )
	{
		part_info * pi = &(*m_pl)[m_ip];
		CString multiple ((LPCSTR) IDS_Multiple);
		m_edit_ref_des.SetWindowText( multiple );
		m_edit_ref_des.EnableWindow( FALSE );
		m_edit_footprint.EnableWindow( m_multiple_mask & MSK_FOOTPRINT );
		part_tree.EnableWindow( m_multiple_mask & MSK_FOOTPRINT );
		m_edit_lib.EnableWindow( m_multiple_mask & MSK_FOOTPRINT );
		m_button_browse.EnableWindow( m_multiple_mask & MSK_FOOTPRINT );
		m_edit_package.EnableWindow( m_multiple_mask & MSK_PACKAGE );
		m_edit_value.EnableWindow( m_multiple_mask & MSK_VALUE );
		m_check_value_visible.EnableWindow( FALSE );
		m_check_ref_visible.EnableWindow( FALSE ); // CPT
		m_edit_package.SetWindowText( "" );
		m_edit_footprint.SetWindowText( "" );
		m_edit_value.SetWindowText( "" );
		m_check_value_visible.SetCheck(0);
		m_check_ref_visible.SetCheck(0);  // CPT

		if( m_units == MIL )
			m_combo_units.SetCurSel(0);
		else
			m_combo_units.SetCurSel(1);
		m_units_initial = m_units;
		m_x = m_x_initial = 0;
		m_y = m_y_initial = 0;
		SetFields();
		m_radio_set.EnableWindow( 0 );
		m_radio_drag.EnableWindow( 0 );
		m_radio_offboard.EnableWindow( 0 );
		m_combo_units.EnableWindow( FALSE );
		m_edit_x.EnableWindow( FALSE );
		m_edit_y.EnableWindow( FALSE );
		m_combo_angle.EnableWindow( FALSE );
		m_list_side.EnableWindow( FALSE );
	}
	else
	{
		part_info * pi = &(*m_pl)[m_ip]; 
		m_edit_ref_des.SetWindowText( pi->ref_des ); 
		m_edit_package.SetWindowText( pi->package );
		m_edit_value.SetWindowText( pi->value );
		m_check_value_visible.SetCheck( pi->value_vis );
		m_check_ref_visible.SetCheck( pi->ref_vis );
		if( pi->shape )
			m_edit_footprint.SetWindowText( pi->shape->m_name );
		if( m_units == MIL )
			m_combo_units.SetCurSel(0);
		else
			m_combo_units.SetCurSel(1);
		m_units_initial = m_units;
		m_x = m_x_initial = pi->x;
		m_y = m_y_initial = pi->y;
		SetFields();
		m_list_side.SetCurSel( pi->side );
		int cent_angle = 0;
		if( pi->shape )
			cent_angle = pi->shape->m_centroid->m_angle;
		int angle = GetReportedAngleForPart( pi->angle, 
			cent_angle, pi->side );
		m_combo_angle.SetCurSel( angle/90 );
		m_radio_drag.SetCheck( 0 );
		m_radio_offboard.SetCheck( 0 );
		m_radio_set.SetCheck( 1 );
		m_radio_drag.EnableWindow( 0 );
		m_radio_offboard.EnableWindow( 0 );
		m_drag_flag = FALSE;
		m_combo_units.EnableWindow( TRUE );
		m_edit_x.EnableWindow( TRUE );
		m_edit_y.EnableWindow( TRUE );
		m_combo_angle.EnableWindow( TRUE );
		m_list_side.EnableWindow( TRUE );
	}
	return TRUE;  // return TRUE unless you set the focus to a control
}

// Initialize the tree control representing the footprint library and cache
//
void CDlgAddPart::InitPartLibTree()
{
	CString str;

	// initialize folder name
	m_edit_lib.SetWindowText( *m_folder->GetFullPath() );
	CTreeCtrl * pCtrl = &part_tree;
	pCtrl->DeleteAllItems();
	int i_exp = 0;
	
	// allow vertical scroll
	long style = ::GetWindowLong( part_tree, GWL_STYLE );
	style = style & ~TVS_NOSCROLL;
	::SetWindowLong( part_tree, GWL_STYLE, style | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS );

	// insert local cache name
	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = NULL;
	tvInsert.hInsertAfter = NULL;
	tvInsert.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvInsert.item.pszText = _T("local cache");
	tvInsert.item.lParam = -1;
	HTREEITEM hLocal = pCtrl->InsertItem(&tvInsert);

	if( gLocalCacheExpanded )
		part_tree.SetItemState( hLocal, TVIS_EXPANDED, TVIS_EXPANDED );

	// insert cached footprints
	CIter<CShape> is (&m_cache_shapes->shapes);
	gTempCachePtrs.RemoveAll();
	int i = 0;
	for (CShape *s = is.First(); s; s = is.Next(), i++)
	{
		tvInsert.hInsertAfter = 0;
		tvInsert.hParent = hLocal;
		tvInsert.item.pszText = (LPSTR)(LPCSTR) s->m_name;
		tvInsert.item.lParam = (LPARAM) i;
		gTempCachePtrs.Add(s);
		pCtrl->InsertItem(&tvInsert);
	}

	// insert all library names
	HTREEITEM hLib;
	HTREEITEM hLib_last;
	HTREEITEM hHead;
	HTREEITEM hHead_last;
	// loop through libraries
	for( int ilib=0; ilib<m_folder->GetNumLibs(); ilib++ )
	{
		// put library filename into Tree
		str = *m_folder->GetLibraryFileName( ilib );
		tvInsert.hParent = NULL;
		tvInsert.item.pszText = (LPSTR)(LPCSTR) str;
		if( ilib == 0 )
			tvInsert.hInsertAfter = hLocal;
		else
			tvInsert.hInsertAfter = hLib_last;
		tvInsert.item.lParam = -1;
		hLib = pCtrl->InsertItem(&tvInsert);	// insert library name

		if( m_folder->GetExpanded( ilib ) )
			part_tree.SetItemState( hLib, TVIS_EXPANDED, TVIS_EXPANDED );

		hLib_last = hLib;

		// loop through footprints in heading
		for( int i=0; i<m_folder->GetNumFootprints(ilib); i++ )
		{
			// put footprint into tree
			str = *m_folder->GetFootprintName( ilib, i );
			tvInsert.hParent = hLib;
			tvInsert.item.pszText = (LPSTR)(LPCSTR) str;
			UINT32 lp = (ilib+1)*0x1000000 + i;
			tvInsert.item.lParam = (LPARAM)lp;
			tvInsert.hInsertAfter = 0;
			pCtrl->InsertItem(&tvInsert);
		}
	}
}

void CDlgAddPart::OnTvnSelchangedPartLibTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	UINT32 lp = pNMTreeView->itemNew.lParam;
	m_ilib = -1;
	m_ihead = -1;
	m_ifoot = -1;
	if( lp != -1 )
	{
		// Determine the library + footprint on the basis of the lParam.
		m_ilib = (lp>>24) & 0xff;
		m_ihead = (lp>>16) & 0xff;
		m_ifoot = lp & 0xffff;
		m_in_cache = m_ilib == 0;
		if( m_in_cache )
			m_shape = gTempCachePtrs[m_ifoot],
			m_shape_file_name = "";
		else
		{
			// not in cache, get from library file
			m_ilib--;
			m_footprint_name = *m_folder->GetFootprintName( m_ilib, m_ifoot );
			m_shape_file_name = *m_folder->GetLibraryFullPath( m_ilib );
			int offset = m_folder->GetFootprintOffset( m_ilib, m_ifoot );
			extern CFreePcbApp theApp;
			m_shape = new CShape (theApp.m_doc);
			int err = m_shape->MakeFromFile( NULL, m_footprint_name, m_shape_file_name, offset ); 
			if( err )
				// unable to make shape
				ASSERT(0);
		}
		m_footprint_name = m_shape->m_name;
		m_edit_footprint.SetWindowText( m_footprint_name );

		// now draw preview of footprint
		CMetaFileDC m_mfDC;
		CDC * pDC = this->GetDC();
		CRect rw;
		m_preview.GetClientRect( &rw );
		int x_size = rw.right - rw.left;
		int y_size = rw.bottom - rw.top;
		HENHMETAFILE hMF = m_shape->CreateMetafile( &m_mfDC, pDC, CRect(0, 0, x_size, y_size) );
		m_preview.SetEnhMetaFile( hMF );
		ReleaseDC( pDC );
		// update text strings
		m_edit_author.SetWindowText( m_shape->m_author );
		m_edit_source.SetWindowText( m_shape->m_source );
		m_edit_desc.SetWindowText( m_shape->m_desc );
	}
	*pResult = 0;
}

void CDlgAddPart::OnBnClickedRadioDrag()
{
	m_combo_units.EnableWindow( FALSE );
	m_edit_x.EnableWindow( FALSE );
	m_edit_y.EnableWindow( FALSE );
	m_combo_angle.EnableWindow( FALSE );
	m_list_side.EnableWindow( FALSE );
	m_drag_flag = TRUE;
}

void CDlgAddPart::OnBnClickedRadioSet()
{
	m_combo_units.EnableWindow( TRUE );
	m_edit_x.EnableWindow( TRUE );
	m_edit_y.EnableWindow( TRUE );
	m_combo_angle.EnableWindow( TRUE );
	m_list_side.EnableWindow( TRUE );
	m_drag_flag = FALSE;
}

void CDlgAddPart::OnBnClickedRadioOffBoard()
{
	m_combo_units.EnableWindow( FALSE );
	m_edit_x.EnableWindow( FALSE );
	m_edit_y.EnableWindow( FALSE );
	m_combo_angle.EnableWindow( FALSE );
	m_list_side.EnableWindow( FALSE );
	m_drag_flag = FALSE;
}

void CDlgAddPart::OnBnClickedOk()
{
	// get state of tree control so we can reproduce it next time
	// get next top-level item
	HTREEITEM item = part_tree.GetNextItem( NULL, TVGN_CHILD );
	// get all items
	int ilib = -1;
	while( item )
	{
		// top-level item
		BOOL expanded = part_tree.GetItemState( item, TVIS_EXPANDED );
		CString str;
		if( ilib == -1 )
			gLocalCacheExpanded = expanded & TVIS_EXPANDED;
		else
			m_folder->SetExpanded( ilib, expanded & TVIS_EXPANDED );
		// get next top-level item
		item = part_tree.GetNextItem( item, TVGN_NEXT );
		ilib++;
	}
	OnOK();
}

void CDlgAddPart::OnBnClickedCancel()
{
	// get state of tree control so we can reproduce it next time
	// get next top-level item
	HTREEITEM item = part_tree.GetNextItem( NULL, TVGN_CHILD );
	// get all items
	int ilib = -1;
	while( item )
	{
		// top-level item
		BOOL expanded = TVIS_EXPANDED & part_tree.GetItemState( item, TVIS_EXPANDED );
		CString str;
		if( ilib == -1 )
			gLocalCacheExpanded = expanded & TVIS_EXPANDED;
		else
			m_folder->SetExpanded( ilib, expanded & TVIS_EXPANDED );
		// get next top-level item
		item = part_tree.GetNextItem( item, TVGN_NEXT );
		ilib++;
	}
	if (m_doc->m_undo_items.GetSize() > 0)
		// CPT2 Could happen if user hit "Compare with netlist" and fixed pins.  So I guess we'll undo that...
		m_doc->UndoNoRedo();
	OnCancel();
}

void CDlgAddPart::OnCbnSelchangeComboAddPartUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgAddPart::GetFields()
{
	CString strX, strY, strXInit, strYInit;
	if( m_units == MIL )
	{
		// CPT2.  Modified this to ensure that we revert to the initial values of m_x and m_y if the text/units are the same as they were initially.
		// Before, rounding errors could cause the part to move a tiny amount. 
		m_edit_x.GetWindowText( strX );
		m_edit_y.GetWindowText( strY );
		MakeCStringFromDouble( &strXInit, m_x_initial/NM_PER_MIL );
		MakeCStringFromDouble( &strYInit, m_y_initial/NM_PER_MIL );
		if (m_units_initial==m_units && strX==strXInit && strY==strYInit)
			m_x = m_x_initial,
			m_y = m_y_initial;
		else
			m_x = atof( strX ) * NM_PER_MIL,
			m_y = atof( strY ) * NM_PER_MIL;
	}
	else
	{
		m_edit_x.GetWindowText( strX );
		m_edit_y.GetWindowText( strY );
		MakeCStringFromDouble( &strXInit, m_x_initial/1000000. );
		MakeCStringFromDouble( &strYInit, m_y_initial/1000000. );
		if (m_units_initial==m_units && strX==strXInit && strY==strYInit)
			m_x = m_x_initial,
			m_y = m_y_initial;
		else
			m_x = atof( strX ) * 1000000.,
			m_y = atof( strY ) * 1000000.;
	}
	m_edit_ref_des.GetWindowTextA( m_ref_des );
}

void CDlgAddPart::SetFields()
{
	CString str;
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, m_x/NM_PER_MIL );
		m_edit_x.SetWindowText( str );
		MakeCStringFromDouble( &str, m_y/NM_PER_MIL );
		m_edit_y.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, m_x/1000000.0 );
		m_edit_x.SetWindowText( str );
		MakeCStringFromDouble( &str, m_y/1000000.0 );
		m_edit_y.SetWindowText( str );
	}
}

void CDlgAddPart::OnBnClickedButtonBrowse()
{
	CString open ((LPCSTR) IDS_OpenFolder), select ((LPCSTR) IDS_SelectFootprintLibraryFolder);
	CPathDialog dlg( open, select, *m_folder->GetFullPath() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString path_str = dlg.GetPathName();
		m_edit_lib.SetWindowText( path_str );
		if( !m_footlibfoldermap->FolderIndexed( &path_str ) )
		{
			// if library folder not indexed, pop up log
			m_dlg_log->ShowWindow( SW_SHOW );   
			m_dlg_log->BringWindowToTop();
			m_dlg_log->Clear();
			m_dlg_log->UpdateWindow(); 
		}
		m_folder = m_footlibfoldermap->GetFolder( &path_str, m_dlg_log );
		if( !m_folder )
		{
			ASSERT(0);
		}
		InitPartLibTree();
		m_footlibfoldermap->SetLastFolder( &path_str );
	}
}

void CDlgAddPart::OnBnClickedButtonCompareWithNetlist()
{
	// CPT2 new, part of the experimental synching-with-netlist-file system
	m_edit_ref_des.GetWindowTextA( m_ref_des );
	if (m_ref_des=="")
	{
		AfxMessageBox (CString ((LPCSTR)IDS_NoReferenceDesignatorHasBeenChosen));
		return;
	}

	CString foot_str;
	m_edit_footprint.GetWindowText( foot_str );
	if (!m_shape || foot_str != m_shape->m_name)
		// Look for the shape named "foot_str" within the cache
		m_shape = m_cache_shapes->GetShapeByName( &foot_str ),
		m_shape_file_name = "";
	if (!m_shape)
	{
		CString str;
		if (foot_str=="")
			str.LoadStringA(IDS_ErrorNoFootprintSelected);
		else
			str.LoadStringA(IDS_ErrorCouldntLocateFootprintName);
		AfxMessageBox( str );
		return;
	}

	m_dlg_log->ShowWindow( SW_SHOW );   
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow(); 
	CString line, str0;
	CPart *part = m_ip>=0? (*m_pl)[m_ip].part: NULL;

	// First check if ref des is found in the sync-file partlist
	int sizePli = m_doc->m_sync_file_pli.GetSize();
	bool bFound = false;
	for (int i=0; i<sizePli; i++)
	{
		part_info *pi = &m_doc->m_sync_file_pli[i];
		if (pi->ref_des == m_ref_des)
		{
			str0.LoadStringA( IDS_FoundPartInNetlistFileWithFootprintName );
			line.Format(str0, m_ref_des, m_doc->m_sync_file, pi->shape_name);
			m_dlg_log->AddLine(line);
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		str0.LoadStringA( IDS_PartNotListedInNetlistFile );
		line.Format(str0, m_ref_des, m_doc->m_sync_file);
		m_dlg_log->AddLine(line);
	}

	// Now see if part's pins are found among the nets in the sync-file netlist
	// Also gather a list of pins that are on the wrong nets, and the corresponding correct nets
	int sizeNli = m_doc->m_sync_file_nli.GetSize();
	CArray<CString> correctPins, correctNets;
	for (int i=0; i<sizeNli; i++)
	{
		net_info *ni = &m_doc->m_sync_file_nli[i];
		for (int j=0; j<ni->ref_des.GetSize(); j++)
		{
			if (ni->ref_des[j] != m_ref_des)
				continue;
			CString pin_name = ni->pin_name[j];
			CPadstack *ps = m_shape->GetPadstackByName( &pin_name );
			if (!ps)
			{
				str0.LoadStringA( IDS_PinFoundInNetlistFileNotPresentWithTheFootprint );
				line.Format(str0, m_ref_des, ni->pin_name[j], foot_str);
				m_dlg_log->AddLine(line);
			}
			else
			{
				CPin *pin = part? part->GetPinByName( &pin_name ): NULL;
				if (!pin)
				{
					// Hmm, user must have changed shape, and the new shape has pin_name while the old one doesn't
					str0.LoadStringA( IDS_PinFoundInNetlistFilePresentWithTheNewFootprint );
					line.Format(str0, m_ref_des, pin_name, foot_str);
					m_dlg_log->AddLine(line);
				}
				else if (!pin->net || pin->net->name != ni->name)
				{
					str0.LoadStringA( IDS_PinFoundInNetlistFilePresentOnBoardButShouldBeAssignedToNet );
					line.Format(str0, m_ref_des, pin_name, ni->name);
					m_dlg_log->AddLine(line);
					correctPins.Add(pin_name);
					correctNets.Add(ni->name);
				}
				else
				{
					str0.LoadStringA( IDS_PinFoundInNetlistFilePresentOnBoardAndCorrectlyAssignedToNet );
					line.Format(str0, m_ref_des, pin_name, ni->name);
					m_dlg_log->AddLine(line);
				}
			}
		}
	}

	if (correctPins.IsEmpty()) return;
	CString s ((LPCSTR) IDS_DoYouWantToFixThePinsThatAreOnTheWrongNets);
	int ret = m_dlg_log->MessageBox(s, "", MB_YESNO);
	if (ret==IDNO) return;

	m_dlg_log->ShowWindow(SW_HIDE);
	m_doc->m_nlist->nets.SetUtility(0);
	for (int i=0; i<correctPins.GetSize(); i++)
	{
		CPin *pin = part->GetPinByName( &correctPins[i] );
		pin->SaveUndoInfo();
		if (pin->net)
			pin->net->SaveUndoInfo( CNet::SAVE_CONNECTS ),
			pin->net->RemovePin(pin);
		CNet *net = m_doc->m_nlist->GetNetPtrByName( &correctNets[i] );
		if( !net )
			net = new CNet(m_doc->m_nlist, correctNets[i], 0, 0, 0);
		else
			net->SaveUndoInfo( CNet::SAVE_NET_ONLY );
		net->AddPin(pin);
		if( m_doc->m_vis[LAY_RAT_LINE] && !net->utility)
			net->OptimizeConnections(),
			net->utility = 1;
	}
}
