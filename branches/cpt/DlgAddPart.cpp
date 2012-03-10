// DlgAddPart.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddPart.h"
#include "resource.h"
#include "DlgDupFootprintName.h"
#include "PathDialog.h"


// save expanded state of local cache
BOOL gLocalCacheExpanded = FALSE;

// global for last ref des
CString last_ref_des = "";
CString last_footprint = "";

// CDlgAddPart dialog

IMPLEMENT_DYNAMIC(CDlgAddPart, CDialog)
CDlgAddPart::CDlgAddPart(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddPart::IDD, pParent)
{
	m_footprint_cache_map = 0;
	m_units = MIL;
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
	if( pDX->m_bSaveAndValidate )
	{
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

		BOOL bFootprintChanged = TRUE;
		CShape * new_shape = NULL;
		void * ptr = 0;
		if( !m_multiple || (m_multiple_mask & MSK_FOOTPRINT) )
		{
			// check if footprint valid and load into cache if necessary
			CString foot_str;
			m_edit_footprint.GetWindowText( foot_str );
			BOOL bInCache = m_footprint_cache_map->Lookup( foot_str, ptr );
			if( bInCache )
			{
				// footprint with the same name is already in the local cache
				// however, it is possible that the footprint was selected from a library
				// file and might not match the footprint in the cache
				CShape * cache_shape = (CShape*)ptr;	// footprint from cache
				BOOL bFootprintUnchanged = cache_shape->Compare( &m_shape );
				if( m_in_cache || bFootprintUnchanged || m_shape.m_name == "EMPTY_SHAPE" )
				{
					// the new footprint is already in the cache, because:
					//	it was selected from the cache, 
					//	or the new footprint is identical to the cached version,
					//	or a new footprint was never selected from the library tree
					// therefore, do nothing
				}
				else
				{
					// load new footprint into cache, 
					// replacing the previous one with the same name
					int num_other_instances = 0;
					for( int i=0; i<m_pl->GetSize(); i++ )
					{
						part_info * pi = &(*m_pl)[i];
						if( pi->shape )
						{
							if( pi->shape->m_name == foot_str && ref_des_str != pi->ref_des )
							{
								num_other_instances++;
							}
						}
					}
					if( num_other_instances )
					{
						// display dialog to warn user about overwriting the old footprint
						CDlgDupFootprintName dlg;
						CString mess, w ((LPCSTR) IDS_WarningAFootprint);
						mess.Format( w, foot_str );
						dlg.Initialize( &mess, m_footprint_cache_map );
						int ret = dlg.DoModal();
						if( ret == IDOK )
						{
							// clicked "OK"
							if( dlg.m_replace_all_flag )
							{
								// replace all instances of footprint
								cache_shape->Copy( &m_shape );
								for( int i=0; i<m_pl->GetSize(); i++ )
								{
									part_info * pi = &(*m_pl)[i];
									if( pi->shape == cache_shape )
									{
										pi->bShapeChanged = TRUE;
									}
								}
							}
							else
							{
								// assign new name to footprint and put in cache
								CShape * shape = new CShape;
								shape->Copy( &m_shape );
								shape->m_name = *dlg.GetNewName();	
								m_footprint_cache_map->SetAt( shape->m_name, shape );
								ptr = shape;
							}
						}
						else
						{
							// clicked "Cancel"
							pDX->Fail();
						}
					}
					else
					{
						// replace the footprint in the cache
						cache_shape->Copy( &m_shape );
						if( !m_new_part && m_ip != -1 )
							(*m_pl)[m_ip].bShapeChanged = TRUE;
					}
				}
			}
			else
			{
				// shape not in cache
				if( m_shape.m_name == "EMPTY_SHAPE" )
				{
					CString msg ((LPCSTR) IDS_ErrorNoFootprintSelected);
					AfxMessageBox( msg );
					pDX->Fail();
				}
				CShape * shape = new CShape;
				shape->Copy( &m_shape );
				shape->m_name = foot_str;	// in case it was renamed
				m_footprint_cache_map->SetAt( foot_str, shape );
				ptr = shape;
			}

			// OK, now we can assume that the footprint is in the cache
			// and ptr is a valid pointer to it
			if( ptr == NULL )
				ASSERT(0);
			new_shape = (CShape*)ptr;

			// check number of pins, unless new part or no old shape
			if( !m_new_part && (*m_pl)[m_ip].shape )
			{
				int n_new_pins = new_shape->m_padstack.GetSize();
				int n_old_pins = (*m_pl)[m_ip].shape->m_padstack.GetSize();
				if( n_new_pins < n_old_pins )
				{
					CString mess, mess0 ((LPCSTR) IDS_WarningSHasFewerPins);
					mess.Format( mess0,	foot_str, (*m_pl)[m_ip].shape->m_name );
					int ret = AfxMessageBox( mess, MB_YESNO );
					if( ret != IDYES)
					{
						m_edit_footprint.SetWindowText( (*m_pl)[m_ip].shape->m_name );
						pDX->Fail();
					}
				}
			}
			if( m_new_part )
			{
				// if new part, add entry to partlist_info
				m_ip = m_pl->GetSize();
				m_pl->SetSize( m_ip + 1 );
				(*m_pl)[m_ip].part = 0;
				// set globals for next time
				last_ref_des = ref_des_str;
				last_footprint = new_shape->m_name;
			}

			// OK, now update partlist_info with new shape
			bFootprintChanged = TRUE;
			if( (*m_pl)[m_ip].shape == new_shape )
				bFootprintChanged = FALSE;
			(*m_pl)[m_ip].shape = new_shape;
			(*m_pl)[m_ip].ref_size = new_shape->m_ref_size;
			(*m_pl)[m_ip].ref_width = new_shape->m_ref_w;
		}


		BOOL bPackageChanged = FALSE;
		CString package_str;
		if( !m_multiple || (m_multiple_mask & MSK_PACKAGE) )
		{
			// update package
			m_edit_package.GetWindowText( package_str );
			if( package_str != (*m_pl)[m_ip].package )
			{
				bPackageChanged = TRUE;
				(*m_pl)[m_ip].package = package_str;
			}
		}

		if( !m_multiple || (m_multiple_mask & MSK_VALUE) )
		{
			// update value
			CString value_str;
			m_edit_value.GetWindowText( value_str );
			(*m_pl)[m_ip].value = value_str;
			if( !m_multiple )
				(*m_pl)[m_ip].value_vis = m_check_value_visible.GetCheck();
		}

		// CPT update reference visibility
		if (!m_multiple)
			(*m_pl)[m_ip].ref_vis = m_check_ref_visible.GetCheck();

		// see if footprints for other parts need to be changed
		if( !m_standalone 
			//			&& !m_multiple 
			&& ( bPackageChanged || bFootprintChanged ) 
			&& package_str != "" )
		{
			CString str, str0 ((LPCSTR) IDS_DoYouWantToAssignFootprint);
			str.Format( str0, (*m_pl)[m_ip].shape->m_name, (*m_pl)[m_ip].package );
			int ret = AfxMessageBox( str, MB_YESNO );
			if( ret == IDYES )
			{
				for( int ip=0; ip<m_pl->GetSize(); ip++ )
				{
					if( (*m_pl)[ip].package == package_str )
					{
						(*m_pl)[ip].shape = (CShape*)ptr;
						(*m_pl)[ip].ref_size = ((CShape*)ptr)->m_ref_size;
						(*m_pl)[ip].ref_width = ((CShape*)ptr)->m_ref_w;
					}
				}
			}
		}

		// update partlist_info
		if( !m_multiple ) 
		{
			// update all fields for part
			GetFields();
			(*m_pl)[m_ip].ref_des = ref_des_str;
			(*m_pl)[m_ip].x = m_x;
			(*m_pl)[m_ip].y = m_y;
			int side = m_list_side.GetCurSel();
			(*m_pl)[m_ip].side = side;
			int cent_angle = 0;
			if( (*m_pl)[m_ip].shape )
				cent_angle = (*m_pl)[m_ip].shape->m_centroid_angle;
			(*m_pl)[m_ip].angle = ::GetPartAngleForReportedAngle( m_combo_angle.GetCurSel()*90, 
				cent_angle, side );
			(*m_pl)[m_ip].deleted = FALSE;
			if( m_radio_offboard.GetCheck() )
				(*m_pl)[m_ip].bOffBoard = TRUE;
			else
				(*m_pl)[m_ip].bOffBoard = FALSE;
		}
	}
	else
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		m_edit_lib.SetWindowText( *m_folder->GetFullPath() );
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
END_MESSAGE_MAP()


//**************** public methods ********************

// initialize dialog
//	
void CDlgAddPart::Initialize( partlist_info * pl, 
							 int i, 
							 BOOL standalone,
							 BOOL new_part,
							 BOOL multiple,
							 int multiple_mask,
							 CMapStringToPtr * shape_cache_map,
							 CFootLibFolderMap * footlibfoldermap,
							 int units,
							 CDlgLog * log )
{
	m_units = units;  
	m_pl = pl;
	m_ip = i;
	m_standalone = standalone;
	m_new_part = new_part;
	m_multiple = multiple;
	m_multiple_mask = multiple_mask;
	m_footprint_cache_map = shape_cache_map;
	m_footlibfoldermap = footlibfoldermap;
	CString * last_folder_path = m_footlibfoldermap->GetLastFolder();
	m_folder = m_footlibfoldermap->GetFolder( last_folder_path, log );
	m_in_cache = FALSE;
	m_dlg_log = log;
}

// get flag indicating that dragging was requested
//
BOOL CDlgAddPart::GetDragFlag()
{
	return m_drag_flag;
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
		m_units = MIL;
		m_combo_units.SetCurSel(0);
		m_x = 0;
		m_y = 0;
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
		m_x = 0;
		m_y = 0;
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
		m_x = (*m_pl)[m_ip].x;
		m_y = (*m_pl)[m_ip].y;
		SetFields();
		m_list_side.SetCurSel( pi->side );
		int cent_angle = 0;
		if( pi->shape )
			cent_angle = pi->shape->m_centroid_angle;
		int angle = GetReportedAngleForPart( (*m_pl)[m_ip].angle, 
			cent_angle, (*m_pl)[m_ip].side );
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
	LPCSTR p;

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
	POSITION pos;
	CString key;
	void * ptr;
	int i = 0;
	for( pos = m_footprint_cache_map->GetStartPosition(); pos != NULL; )
	{
		m_footprint_cache_map->GetNextAssoc( pos, key, ptr );
		p = (LPCSTR)key;
		tvInsert.hInsertAfter = 0;
		tvInsert.hParent = hLocal;
		tvInsert.item.pszText = (LPSTR)p;
		tvInsert.item.lParam = (LPARAM)i;
		pCtrl->InsertItem(&tvInsert);
		i++;
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
		p = (LPCSTR)str;
		tvInsert.hParent = NULL;
		tvInsert.item.pszText = (LPSTR)p;
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
			p = (LPCSTR)str;
			tvInsert.hParent = hLib;
			tvInsert.item.pszText = (LPSTR)p;
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
		m_ilib = (lp>>24) & 0xff;
		m_ihead = (lp>>16) & 0xff;
		m_ifoot = lp & 0xffff;
		CString str = "";
		if( m_ilib == 0 )
		{
			m_in_cache = TRUE;
			POSITION pos;
			CString key;
			void * ptr;
			pos = m_footprint_cache_map->GetStartPosition();
			for( int i=0; i<=m_ifoot; i++ )
			{
				m_footprint_cache_map->GetNextAssoc( pos, key, ptr );
			}
			str = key;
		}
		else
		{
			m_ilib--;
			m_in_cache = FALSE;
			str = *m_folder->GetFootprintName( m_ilib, m_ifoot );
		}
		m_footprint_name = str;
		m_edit_footprint.SetWindowText( str );

		// draw footprint preview in control
		void * ptr;
		// lookup shape in cache
		if( m_in_cache )
		{
			// found it, make shape
			BOOL found = m_footprint_cache_map->Lookup( m_footprint_name, ptr );
			if( !found )
				ASSERT(0);
			m_shape.Copy( (CShape*)ptr );
		}
		else
		{
			// not in cache, get from library file
			CString * lib_file_name = m_folder->GetLibraryFullPath( m_ilib );
			int offset = m_folder->GetFootprintOffset( m_ilib, m_ifoot );
			// make shape from library file
			int err = m_shape.MakeFromFile( NULL, m_footprint_name, *lib_file_name, offset ); 
			if( err )
			{
				// unable to make shape
				ASSERT(0);
			}
		}
		// now draw preview of footprint
		CMetaFileDC m_mfDC;
		CDC * pDC = this->GetDC();
		CRect rw;
		m_preview.GetClientRect( &rw );
		int x_size = rw.right - rw.left;
		int y_size = rw.bottom - rw.top;
		HENHMETAFILE hMF = m_shape.CreateMetafile( &m_mfDC, pDC, x_size, y_size );
		m_preview.SetEnhMetaFile( hMF );
		ReleaseDC( pDC );
		// update text strings
		m_edit_author.SetWindowText( m_shape.m_author );
		m_edit_source.SetWindowText( m_shape.m_source );
		m_edit_desc.SetWindowText( m_shape.m_desc );
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
	CString str;
	if( m_units == MIL )
	{
		m_edit_x.GetWindowText( str );
		m_x = atof( str ) * NM_PER_MIL;
		m_edit_y.GetWindowText( str );
		m_y = atof( str ) * NM_PER_MIL;
	}
	else
	{
		m_edit_x.GetWindowText( str );
		m_x = atof( str ) * 1000000.0;
		m_edit_y.GetWindowText( str );
		m_y = atof( str ) * 1000000.0;
	}
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
			m_dlg_log->UpdateWindow();
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

