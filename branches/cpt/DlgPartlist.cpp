// DlgPartlist.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPartlist.h"
#include "DlgAddPart.h"
#include "Part.h"

//global partlist_info so that sorting callbacks will work.  CPT2:  now a static in the CDlgPartlist namespace.
partlist_info CDlgPartlist::pli;
int gDrawRow = -1, gDrawCol = -1, gDrawRowTmp, gDrawColTmp;

// columns for list
enum {
	COL_REFVIS = 0,
	COL_VALVIS,
	COL_NAME,
	COL_PACKAGE,
	COL_FOOTPRINT,
	COL_VALUE
};

// sort types
enum {
	SORT_UP_NAME = 0,
	SORT_DOWN_NAME,
	SORT_UP_PACKAGE,
	SORT_DOWN_PACKAGE,
	SORT_UP_FOOTPRINT,
	SORT_DOWN_FOOTPRINT,
	SORT_UP_VALUE,
	SORT_DOWN_VALUE
};

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//		
int CALLBACK ComparePartlist( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	int ret = 0;
	partlist_info &pli = CDlgPartlist::pli;
	switch( type )
	{
		case SORT_UP_NAME:
		case SORT_DOWN_NAME:
			ret = CompareNumeric( &pli[lp1].ref_des, &pli[lp2].ref_des );
			break;

		case SORT_UP_PACKAGE:
		case SORT_DOWN_PACKAGE:
			ret = (strcmp( pli[lp1].package, pli[lp2].package ));
			break;

		case SORT_UP_FOOTPRINT:
		case SORT_DOWN_FOOTPRINT:
			if( pli[lp1].shape && pli[lp2].shape )
				ret = (strcmp( pli[lp1].shape->m_name, pli[lp2].shape->m_name ));
			else
				ret = 0;
			break;

		case SORT_UP_VALUE:
		case SORT_DOWN_VALUE:
			ret = (strcmp( pli[lp1].value, pli[lp2].value ));
			break;

	}
	switch( type )
	{
		case SORT_DOWN_NAME:
		case SORT_DOWN_PACKAGE:
		case SORT_DOWN_FOOTPRINT:
		case SORT_DOWN_VALUE:
			ret = -ret;
			break;
	}
	return ret;
}

// CDlgPartlist dialog

IMPLEMENT_DYNAMIC(CDlgPartlist, CDialog)
CDlgPartlist::CDlgPartlist(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPartlist::IDD, pParent)
{
}

CDlgPartlist::~CDlgPartlist()
{
}

void CDlgPartlist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list_ctrl);
	DDX_Control(pDX, IDOK, m_button_ok);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add);
	DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_edit);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_delete);
	DDX_Control(pDX, IDC_CHECK1, m_check_footprint);
	DDX_Control(pDX, IDC_CHECK2, m_check_package);
	DDX_Control(pDX, IDC_CHECK3, m_check_value);
}


BEGIN_MESSAGE_MAP(CDlgPartlist, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnLvnColumnClickList1)
	ON_BN_CLICKED(IDC_BUTTON_VAL_VIS, OnBnClickedValueVisible)
	ON_BN_CLICKED(IDC_BUTTON_VAL_INVIS, OnBnClickedValueInvisible)
	ON_BN_CLICKED(IDC_BUTTON_REF_VIS, OnBnClickedRefVisible)
	ON_BN_CLICKED(IDC_BUTTON_REF_INVIS, OnBnClickedRefInvisible)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, OnNMClickList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnNMDblClickList1)
END_MESSAGE_MAP()

BOOL CDlgPartlist::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_plist->ExportPartListInfo( &pli, NULL );
	m_sort_type = SORT_UP_NAME;
	DrawListCtrl(true);
	m_check_footprint.EnableWindow(0);
	m_check_package.EnableWindow(0);
	m_check_value.EnableWindow(0);
	return TRUE;
}

void CDlgPartlist::DrawListCtrl(bool bSetup)
{
	// set up listview control, or just refresh the display if bSetup is false.
	int nItem;
	LVITEM lvitem;
	CString str;
	if (bSetup)
	{
		DWORD old_style = m_list_ctrl.GetExtendedStyle();
		m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | old_style );
		m_list_ctrl.DeleteAllItems();
		CString colNames[6];
		for (int i=0; i<6; i++)
			colNames[i].LoadStringA(IDS_PartListCols2+i);
		m_list_ctrl.InsertColumn( COL_REFVIS, colNames[0], LVCFMT_LEFT, 50 );
		m_list_ctrl.InsertColumn( COL_VALVIS, colNames[1], LVCFMT_LEFT, 55 );
		m_list_ctrl.InsertColumn( COL_NAME, colNames[2], LVCFMT_LEFT, 70 );
		m_list_ctrl.InsertColumn( COL_PACKAGE, colNames[3], LVCFMT_LEFT, 150 );
		m_list_ctrl.InsertColumn( COL_FOOTPRINT, colNames[4], LVCFMT_LEFT, 150 );
		m_list_ctrl.InsertColumn( COL_VALUE, colNames[5], LVCFMT_LEFT, 150 );
	}
	else
		m_list_ctrl.DeleteAllItems();

	for( int i=0; i<pli.GetSize(); i++ )
	{
		lvitem.mask = LVIF_TEXT | LVIF_PARAM;
		lvitem.pszText = "";
		lvitem.lParam = i;
		nItem = m_list_ctrl.InsertItem( i, "" );
		m_list_ctrl.SetItemData( i, (LPARAM)i );
		ListView_SetCheckState( m_list_ctrl, nItem, pli[i].value_vis );
		m_list_ctrl.SetItem( i, COL_NAME, LVIF_TEXT, pli[i].ref_des, 0, 0, 0, 0 );
		m_list_ctrl.SetItem( i, COL_PACKAGE, LVIF_TEXT, pli[i].package, 0, 0, 0, 0 );
		if( pli[i].shape )
			m_list_ctrl.SetItem( i, COL_FOOTPRINT, LVIF_TEXT, pli[i].shape->m_name, 0, 0, 0, 0 );
		else
			m_list_ctrl.SetItem( i, COL_FOOTPRINT, LVIF_TEXT, "", 0, 0, 0, 0 );
		m_list_ctrl.SetItem( i, COL_VALUE, LVIF_TEXT, pli[i].value, 0, 0, 0, 0 );
	}
	m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );	// resort 
	RestoreSelections();
}

void CDlgPartlist::Initialize( CPartList * plist,
			CShapeList * cache_shapes,
			CFootLibFolderMap * footlibfoldermap,
			int units, CDlgLog * log )
{
	m_units = units;
	m_plist = plist;
	m_cache_shapes = cache_shapes;
	m_footlibfoldermap = footlibfoldermap;
	m_sort_type = SORT_UP_NAME;
	m_dlg_log = log;
}

// CDlgPartlist message handlers

void CDlgPartlist::OnBnClickedButtonEdit()
{
	SaveSelections();
	// edit selected part(s)
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 ) 
	{
		AfxMessageBox( CString((LPCSTR) IDS_YouHaveNoPartSelected) );
		return;
	}
	BOOL bMultiple = n_sel > 1;
	int multiple_mask = MSK_FOOTPRINT * m_check_footprint.GetCheck()
					+ MSK_PACKAGE * m_check_package.GetCheck()
					+ MSK_VALUE * m_check_value.GetCheck(); 
	if( bMultiple && multiple_mask == 0 )
	{
		CString s ((LPCSTR) IDS_ToEditMultipleParts);
		AfxMessageBox( s );
		return;
	}

	POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition(); 
	if (pos == NULL)
		ASSERT(0);
	int iItem = m_list_ctrl.GetNextSelectedItem(pos);
	int i = m_list_ctrl.GetItemData( iItem );
	CShape *oldShape = pli[i].shape;
	CString oldPackage = pli[i].package;
	CDlgAddPart dlg;
	dlg.Initialize( &pli, i, FALSE, FALSE, bMultiple, multiple_mask,
		m_cache_shapes, m_footlibfoldermap, m_units, m_dlg_log );
	int ret = dlg.DoModal();
	m_button_ok.SetFocus();
	if( ret != IDOK )
		return;

	if( bMultiple )
	{
		// update all selected parts with new package/footprint/value
		POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
		while( pos )
		{
			int iItem = m_list_ctrl.GetNextSelectedItem(pos);
			int ip = m_list_ctrl.GetItemData( iItem );
			if( ip == i ) 
				continue;
			if( multiple_mask & MSK_FOOTPRINT )
			{
				pli[ip].shape = pli[i].shape;
				pli[ip].shape_file_name = pli[i].shape_file_name;
				pli[ip].ref_size = pli[i].ref_size;
				pli[ip].ref_width = pli[i].ref_width;
				pli[ip].value_size = pli[i].value_size;
				pli[ip].value_width = pli[i].value_width;
			}
			if( multiple_mask & MSK_PACKAGE )
				pli[ip].package = pli[i].package;
			if( multiple_mask & MSK_VALUE )
				pli[ip].value = pli[i].value;
		}
	}

	// CPT2.  See if there are any parts in pli that have the same package as that chosen during the DlgAddPart, but 
	// different footprints.  If so, offer the option to assign the chosen footprint to those other parts:
	// (code that more or less did this used to be located in CDlgAddPart::DoDataExchange())
	bool bMustAsk = true;
	if (pli[i].package != "" && (pli[i].package != oldPackage || pli[i].shape != oldShape))
		for (int j=0; j<pli.GetSize(); j++)
		{
			if (pli[j].package != pli[i].package || pli[j].shape == pli[i].shape)
				continue;
			if (bMustAsk)
			{
				CString str, str0 ((LPCSTR) IDS_DoYouWantToAssignFootprint);
				str.Format( str0, pli[i].shape->m_name, pli[i].package );
				int ret = AfxMessageBox( str, MB_YESNO );
				if (ret == IDNO)
					break;
				bMustAsk = false;
			}
			pli[j].shape = pli[i].shape;
			pli[j].ref_size = pli[i].shape->m_ref->m_font_size;				// I guess (what a lot of fuss)...
			pli[j].ref_width = pli[i].shape->m_ref->m_stroke_width;
			pli[j].value_size = pli[i].shape->m_value->m_font_size;	
			pli[j].value_width = pli[i].shape->m_value->m_stroke_width;
		}

	DrawListCtrl(false);
}

void CDlgPartlist::OnBnClickedButtonAdd()
{
	SaveSelections();
	// now add part
	CDlgAddPart dlg;
	dlg.Initialize( &pli, -1, FALSE, TRUE, FALSE, 0,
		m_cache_shapes, m_footlibfoldermap, m_units, m_dlg_log );
	int ret = dlg.DoModal();
	m_button_ok.SetFocus();
	if( ret == IDOK )
		DrawListCtrl(false);
}

void CDlgPartlist::OnBnClickedButtonDelete()
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 ) {
		CString s ((LPCSTR) IDS_YouHaveNoPartSelected);
		AfxMessageBox( s );
	}
	else
	{
		while( m_list_ctrl.GetSelectedCount() )
		{
			POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
			if (pos == NULL)
				ASSERT(0);
			int iItem = m_list_ctrl.GetNextSelectedItem(pos);
			int ip = m_list_ctrl.GetItemData( iItem );
			pli[ip].deleted = TRUE;
			m_list_ctrl.DeleteItem( iItem );
		}
	}
	m_button_ok.SetFocus();
}

// set m_sort_type based on column clicked and last sort, 
// then sort the list, then save m_last_sort_type = m_sort_type
//
void CDlgPartlist::OnLvnColumnClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int column = pNMLV->iSubItem;
	if( column == COL_NAME )
	{
		if( m_sort_type == SORT_UP_NAME )
			m_sort_type = SORT_DOWN_NAME;
		else
			m_sort_type = SORT_UP_NAME;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	else if( column == COL_PACKAGE )
	{
		if( m_sort_type == SORT_UP_PACKAGE )
			m_sort_type = SORT_DOWN_PACKAGE;
		else
			m_sort_type = SORT_UP_PACKAGE;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	else if( column == COL_FOOTPRINT )
	{
		if( m_sort_type == SORT_UP_FOOTPRINT )
			m_sort_type = SORT_DOWN_FOOTPRINT;
		else
			m_sort_type = SORT_UP_FOOTPRINT;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	else if( column == COL_VALUE )
	{
		if( m_sort_type == SORT_UP_VALUE )
			m_sort_type = SORT_DOWN_VALUE;
		else
			m_sort_type = SORT_UP_VALUE;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	*pResult = 0;
}

void CDlgPartlist::OnVisibleButton(bool bVis, bool bValue)
{
	SaveSelections();
	POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
	while( pos )
	{
		int iItem = m_list_ctrl.GetNextSelectedItem(pos);
		int ip = m_list_ctrl.GetItemData( iItem );
		if (bValue)
			pli[ip].value_vis = bVis;
		else
			pli[ip].ref_vis = bVis;
	}
	DrawListCtrl(false);
	RestoreSelections();
	m_button_ok.SetFocus();
}

void CDlgPartlist::OnBnClickedValueVisible()
	{ OnVisibleButton(true, true); }

void CDlgPartlist::OnBnClickedValueInvisible()
	{ OnVisibleButton(false, true); }

void CDlgPartlist::OnBnClickedRefVisible()
	{ OnVisibleButton(true, false); }

void CDlgPartlist::OnBnClickedRefInvisible()
	{ OnVisibleButton(false, false); }


void CDlgPartlist::OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// CPT2 first deal with clicks in my 2 custom checkbox columns (cols 0 and 1)
	NMITEMACTIVATE *pIA = (NMITEMACTIVATE*)pNMHDR;
	LVHITTESTINFO info;
	info.pt = pIA->ptAction;
	info.iItem = info.iSubItem = -1;
	ListView_SubItemHitTest(pNMHDR->hwndFrom, &info);
	int row = info.iItem, col = info.iSubItem;
	if (col==0 || col==1)
	{
		int dataRow = m_list_ctrl.GetItemData(row);
		if (info.iSubItem==0)
			pli[dataRow].ref_vis = !pli[dataRow].ref_vis;
		else
			pli[dataRow].value_vis = !pli[dataRow].value_vis;
		gDrawRow = row; gDrawCol = col;								// When we redraw in a sec, just do this row/col
		m_list_ctrl.Invalidate(false);
	}

	static int last_n_sel = 0;
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel != last_n_sel )
	{
		m_check_footprint.EnableWindow( n_sel > 1 );
		m_check_package.EnableWindow( n_sel > 1 );
		m_check_value.EnableWindow( n_sel > 1 );
	}
	last_n_sel = n_sel;
	*pResult = 0;
}

void CDlgPartlist::OnNMDblClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnBnClickedButtonEdit();
}

void CDlgPartlist::SaveSelections()
{
	int nItems = m_list_ctrl.GetItemCount();
	bSelected.SetSize( pli.GetSize() );
	for( int iItem=0; iItem<nItems; iItem++ )
	{
		int ip = m_list_ctrl.GetItemData( iItem );
		if( m_list_ctrl.GetItemState( iItem, LVIS_SELECTED ) == LVIS_SELECTED )
			bSelected[ip] = TRUE;
		else
			bSelected[ip] = FALSE;
	}
}

void CDlgPartlist::RestoreSelections()
{

	int nItems = m_list_ctrl.GetItemCount();
	for( int iItem=0; iItem<nItems; iItem++ )
	{
		int ip = m_list_ctrl.GetItemData( iItem );
		if( ip < bSelected.GetSize() && bSelected[ip] )
			m_list_ctrl.SetItemState( iItem, LVIS_SELECTED, LVIS_SELECTED );
	}
	bSelected.SetSize(0);
}


// CPT2.  New class CMyListCtrl so that we can have a custom-draw listview with two checkboxes per row.  What a pain...

IMPLEMENT_DYNAMIC(CMyListCtrl, CListCtrl)

BEGIN_MESSAGE_MAP(CMyListCtrl, CListCtrl)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
END_MESSAGE_MAP()


void CMyListCtrl::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW *pCD = (NMLVCUSTOMDRAW*)pNMHDR;
	int iRow = pCD->nmcd.dwItemSpec;
	int iColumn = pCD->iSubItem;

	switch( pCD->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			gDrawRowTmp = gDrawRow; gDrawColTmp = gDrawCol;
			gDrawRow = -1; gDrawCol = -1;							// Next time we paint, we'll draw everything for sure (unless user overrides)
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			*pResult = CDRF_NOTIFYSUBITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT | CDDS_SUBITEM :
			if (gDrawRowTmp>=0 && gDrawColTmp>=0)
				if (iColumn==gDrawRowTmp && iRow==gDrawColTmp)
					*pResult = iColumn<2? CDRF_SKIPDEFAULT: CDRF_DODEFAULT;
				else
					*pResult = CDRF_SKIPDEFAULT;
			else
				*pResult = iColumn<2? CDRF_SKIPDEFAULT: CDRF_DODEFAULT;
			if (iColumn < 2) 
			{
				CRect rect;
				if (iColumn==0)
					// Workaround for the fact that GetSubItemRect comes out wrong for column 0 (it gives the bounds of the whole row):
					GetSubItemRect( iRow, 1, LVIR_BOUNDS, rect),		
					rect.right = rect.left,
					rect.left = 0;
				else
					GetSubItemRect( iRow,iColumn, LVIR_BOUNDS, rect);
				CDC *cdc = CDC::FromHandle(pCD->nmcd.hdc);
				CBrush br (0xffffff);
				CPen pen (PS_SOLID, 2, 0xffffff);
				cdc->SelectObject(br);
				cdc->SelectObject(pen);
				cdc->Rectangle( &rect );
				int dataRow = GetItemData(iRow);
				int buttonState;
				if (iColumn==0)
					buttonState = CDlgPartlist::pli[dataRow].ref_vis? DFCS_BUTTONCHECK|DFCS_CHECKED: DFCS_BUTTONCHECK;
				else
					buttonState = CDlgPartlist::pli[dataRow].value_vis? DFCS_BUTTONCHECK|DFCS_CHECKED: DFCS_BUTTONCHECK;
				DrawFrameControl(pCD->nmcd.hdc, &rect, DFC_BUTTON, buttonState);
			}
			break;
	}
}

