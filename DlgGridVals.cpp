// DlgGridVals.cpp : implementation file.  ALL CPT.

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgGridVals.h"
#include "MyToolBar.h"
#include "DlgAddGridVal.h"
#include "afxdialogex.h"


// CDlgGridVals dialog

CArray<double> gValTable;					// Global static containing all grid values in the main list (both the visible and hidden ones)

IMPLEMENT_DYNAMIC(CDlgGridVals, CDialogEx)

CDlgGridVals::CDlgGridVals(CArray<double> *arr0, CArray<double> *hidden0, int titleRsrc, CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgGridVals::IDD, pParent) {
	arr = arr0;
	arrCopy = new CArray<double>;
	arrCopy->Copy(*arr0);
	hidden = hidden0;
	hiddenCopy = new CArray<double>;
	hiddenCopy->Copy(*hidden0);
	title.LoadStringA(titleRsrc);
	}

CDlgGridVals::~CDlgGridVals()
	{ delete arrCopy; delete hiddenCopy; }

void CDlgGridVals::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_GRIDVALS, m_list_ctrl);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_delete);
	DDX_Control(pDX, IDC_CHECK_GRIDVALS_DEFAULT, m_check_default);

	SetWindowText(title);

	if( pDX->m_bSaveAndValidate )	{
		// update arr and hidden, given the changed check-marks.
		arr->RemoveAll(); 
		hidden->RemoveAll();
		for( int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ ) {
			int i = m_list_ctrl.GetItemData( iItem );
			double val = gValTable[i];
			if (val==0) continue;												// Weird...
			if (ListView_GetCheckState( m_list_ctrl, iItem ))
				arr->Add(val);
			else
				hidden->Add(val);
			}
		bSetDefault = m_check_default.GetCheck();
		} //
	}


BEGIN_MESSAGE_MAP(CDlgGridVals, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedDelete)
	ON_NOTIFY(NM_CLICK, IDC_LIST_GRIDVALS, OnNMClickListNet)
END_MESSAGE_MAP()


// CDlgGridVals message handlers

void CDlgGridVals::OnBnClickedAdd() {
	// prepare and display CDlgAddGridVal
	CDlgAddGridVal dlg;
	int ret = dlg.DoModal();
	if( ret == IDOK ) {
		if (dlg.result==0) {
			CString s ((LPCSTR) IDS_IllegalValue);
			AfxMessageBox( s );
			return;
			}
		for (int i=0; i<gValTable.GetSize(); i++) 
			if (dlg.result==gValTable[i]) {
				CString s ((LPCSTR) IDS_ValueIsAlreadyInTheList);
				AfxMessageBox( s );
				return;
				}
		
		// Get updated versions of arrCopy and hiddenCopy.  Then add the new value to arrCopy.
		arrCopy->RemoveAll();
		hiddenCopy->RemoveAll();
		for( int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ ) {
			int i = m_list_ctrl.GetItemData( iItem );
			double val = gValTable[i];
			if (val==0) continue;												// Weird...
			if (ListView_GetCheckState( m_list_ctrl, iItem ))
				arrCopy->Add(val);
			else
				hiddenCopy->Add(val);
			}
		arrCopy->Add(dlg.result);

		// value added, update m_list_ctrl
		DrawListCtrl();
		}
	}

void CDlgGridVals::OnBnClickedDelete() {
		while( m_list_ctrl.GetSelectedCount())	{
			POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
			int i_sel = m_list_ctrl.GetNextSelectedItem( pos );
			int i = m_list_ctrl.GetItemData( i_sel );
			gValTable[i] = 0;
			m_list_ctrl.DeleteItem( i_sel );
			}
	}

void CDlgGridVals::OnNMClickListNet(NMHDR *pNMHDR, LRESULT *pResult) {
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
		m_button_delete.EnableWindow(FALSE);
	else 
		m_button_delete.EnableWindow(TRUE);
	if( pResult )
		*pResult = 0;
	}

int CALLBACK CompareGridVals( LPARAM lp1, LPARAM lp2, LPARAM type ) {
	// Used when sorting the main listview.  Params lp1 and lp2 will be indices into global gValTable (setup by DrawListCtrl() below)
	return CompareGridVals(&gValTable[lp1], &gValTable[lp2]);
	}

void CDlgGridVals::DrawListCtrl() {
	CString str;
	DWORD old_style = m_list_ctrl.GetExtendedStyle();
	m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | LVS_EX_CHECKBOXES | old_style );
	m_list_ctrl.DeleteAllItems();
	CString colNames[2];
	for (int i=0; i<2; i++)
		colNames[i].LoadStringA(IDS_GridValsCols+i);

	m_list_ctrl.InsertColumn( 0, colNames[0], LVCFMT_LEFT, 70 );
	m_list_ctrl.InsertColumn( 1, colNames[1], LVCFMT_LEFT, 400 );
	gValTable.RemoveAll();
	int cArr = arrCopy->GetSize();
	for (int i=0; i<cArr; i++ ) {
		int nItem = m_list_ctrl.InsertItem( 0, "" );
		m_list_ctrl.SetItemData( 0, (LPARAM)i );
		double val = arrCopy->GetAt(i);
		gValTable.Add(val);
		MakeCStringFromGridVal(&str, val );
		m_list_ctrl.SetItem( 0, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
		ListView_SetCheckState( m_list_ctrl, nItem, true );
		} 
	for (int i=0; i<hiddenCopy->GetSize(); i++ ) {
		int nItem = m_list_ctrl.InsertItem( 0, "" );
		m_list_ctrl.SetItemData( 0, (LPARAM) (cArr+i) );
		double val = hiddenCopy->GetAt(i);
		gValTable.Add(val);
		MakeCStringFromGridVal(&str, val );
		m_list_ctrl.SetItem( 0, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
		ListView_SetCheckState( m_list_ctrl, nItem, false );
		} 
	m_list_ctrl.SortItems(CompareGridVals, 0);

	}
	

BOOL CDlgGridVals::OnInitDialog() {
	CDialog::OnInitDialog();

	DrawListCtrl();
	m_button_delete.EnableWindow(FALSE);

	// initialize buttons
	/* m_button_edit.EnableWindow(FALSE);
	m_button_delete_single.EnableWindow(FALSE);
	m_button_nl_width.EnableWindow(FALSE);
	m_button_delete.EnableWindow(FALSE);
	*/
	return TRUE;  
}
