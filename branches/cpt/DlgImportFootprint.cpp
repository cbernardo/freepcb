// DlgImportFootprint.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgImportFootprint.h"
#include "PathDialog.h"

//globals
extern CString gLastFileName;		// last file name imported
extern CString gLastFolderName;		// last folder name imported
extern BOOL gLocalCacheExpanded;
extern CArray<CShape*> gTempCachePtrs;


// CDlgImportFootprint dialog

IMPLEMENT_DYNAMIC(CDlgImportFootprint, CDialog)
CDlgImportFootprint::CDlgImportFootprint(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgImportFootprint::IDD, pParent)
{
	m_footlibfolder = NULL;
	m_footprint_name = "";
	m_footprint_filename = "";
	m_footprint_folder = "";
	m_shape = NULL;
}

CDlgImportFootprint::~CDlgImportFootprint()
{
}

void CDlgImportFootprint::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_BROWSE_LIB_FOLDER, m_button_browse);
	DDX_Control(pDX, IDC_EDIT_LIB_FOLDER, m_edit_library_folder);
	DDX_Control(pDX, IDC_PART_LIB_TREE, part_tree);
	DDX_Control(pDX, IDC_PREVIEW, m_preview);
	DDX_Control(pDX, IDC_EDIT_IMPORT_AUTHOR, m_edit_author);
	DDX_Control(pDX, IDC_EDIT_IMPORT_SOURCE, m_edit_source);
	DDX_Control(pDX, IDC_EDIT_IMPORT_DESC, m_edit_desc);
}


BEGIN_MESSAGE_MAP(CDlgImportFootprint, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_LIB_FOLDER, OnBnClickedButtonBrowseLibFolder)
	ON_NOTIFY(TVN_SELCHANGED, IDC_PART_LIB_TREE, OnTvnSelchangedPartLibTree)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CDlgImportFootprint message handlers


void CDlgImportFootprint::InitInstance( CShapeList * cache_shapes,
							 CFootLibFolderMap * foldermap, CDlgLog * log )
{
	extern CFreePcbApp theApp;
	m_shape = new CShape ( theApp.m_doc );
	m_cache_shapes = cache_shapes;
	m_foldermap = foldermap;
	CString * path_str = foldermap->GetLastFolder();
	m_footlibfolder = foldermap->GetFolder( path_str, log );
	m_dlg_log = log;
}

BOOL CDlgImportFootprint::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitPartLibTree();
	return TRUE;  // return TRUE unless you set the focus to a control
}


void CDlgImportFootprint::OnBnClickedButtonBrowseLibFolder()
{
	static CString path = "", s1 ((LPCSTR) IDS_OpenFolder), s2 ((LPCSTR) IDS_SelectFootprintLibraryFolder);
	CPathDialog dlg( s1, s2, *m_footlibfolder->GetFullPath() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString path_str = dlg.GetPathName();
		m_edit_library_folder.SetWindowText( path_str );
		m_footlibfolder = m_foldermap->GetFolder( &path_str, m_dlg_log );
		if( !m_footlibfolder )
		{
			ASSERT(0);
		}
		InitPartLibTree();
		m_foldermap->SetLastFolder( &path_str );
	}
}

void CDlgImportFootprint::OnTvnSelchangedPartLibTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	UINT32 lp = pNMTreeView->itemNew.lParam;
	LPSTR text = pNMTreeView->itemNew.pszText;
	m_ilib = -1;
	m_ihead = -1;
	m_ifoot = -1;
	if( lp != -1 )
	{
		// Determine the library + footprint name on the basis of the lParam.
		m_ilib = (lp>>24) & 0xff;
		m_ihead = (lp>>16) & 0xff;
		m_ifoot = lp & 0xffff;
		m_in_cache = m_ilib == 0;
		if( m_in_cache )
			m_shape->Copy( gTempCachePtrs[m_ifoot] ),
			m_footprint_filename = "",
			m_footprint_folder = "";
		else
		{
			m_ilib--;
			CString * lib_file_name = m_footlibfolder->GetLibraryFullPath( m_ilib );
			int offset = m_footlibfolder->GetFootprintOffset( m_ilib, m_ifoot );
			// make shape from library file
			CString name = *m_footlibfolder->GetFootprintName( m_ilib, m_ifoot );
			int err = m_shape->MakeFromFile( NULL, name, *lib_file_name, offset ); 
			if( err )
				// unable to make shape
				ASSERT(0);
			BOOL bOK = ::SplitString( lib_file_name, &m_footprint_folder, &m_footprint_filename, '\\', TRUE );
		}
		m_footprint_name = m_shape->m_name;
	
		// now draw preview of footprint
		CMetaFileDC m_mfDC;
		CDC * pDC = this->GetDC();
		CRect rw;
		m_preview.GetClientRect( &rw );
		HENHMETAFILE hMF = m_shape->CreateMetafile( &m_mfDC, pDC, rw );
		m_preview.SetEnhMetaFile( hMF );
		ReleaseDC( pDC );
		DeleteEnhMetaFile( hMF );

		// update text strings
		m_edit_author.SetWindowText( m_shape->m_author );
		m_edit_source.SetWindowText( m_shape->m_source );
		m_edit_desc.SetWindowText( m_shape->m_desc );
	}
	*pResult = 0;
}

// Initialize the tree control representing the footprint library and cache
//
void CDlgImportFootprint::InitPartLibTree()
{
	CString str;

	// initialize folder name
	m_edit_library_folder.SetWindowText( *m_footlibfolder->GetFullPath() );
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
	int i = 0;
	gTempCachePtrs.RemoveAll();
	for (CShape *s = is.First(); s; s = is.Next(), i++)
	{
		tvInsert.hInsertAfter = 0;
		tvInsert.hParent = hLocal;
		tvInsert.item.pszText = (LPSTR)(LPCSTR) (s->m_name);
		tvInsert.item.lParam = (LPARAM)i;
		gTempCachePtrs.Add( s );
		pCtrl->InsertItem(&tvInsert);
	}

	// insert all library names
	HTREEITEM hLib;
	HTREEITEM hLib_last;
	HTREEITEM hHead;
	HTREEITEM hHead_last;
	// loop through libraries
	for( int ilib=0; ilib<m_footlibfolder->GetNumLibs(); ilib++ )
	{
		// put library filename into Tree
		str = *m_footlibfolder->GetLibraryFileName( ilib );
		tvInsert.hParent = NULL;
		tvInsert.item.pszText = (LPSTR)(LPCSTR) str;
		if( ilib == 0 )
			tvInsert.hInsertAfter = hLocal;
		else
			tvInsert.hInsertAfter = hLib_last;
		tvInsert.item.lParam = -1;
		hLib = pCtrl->InsertItem(&tvInsert);	// insert library name

		if( m_footlibfolder->GetExpanded( ilib ) )
			part_tree.SetItemState( hLib, TVIS_EXPANDED, TVIS_EXPANDED );

		hLib_last = hLib;

		// loop through footprints in heading
		for( int i=0; i<m_footlibfolder->GetNumFootprints(ilib); i++ )
		{
			// put footprint into tree
			str = *m_footlibfolder->GetFootprintName( ilib, i );
			tvInsert.hParent = hLib;
			tvInsert.item.pszText = (LPSTR)(LPCSTR) str;
			UINT32 lp = (ilib+1)*0x1000000 + i;
			tvInsert.item.lParam = (LPARAM)lp;
			tvInsert.hInsertAfter = 0;
			pCtrl->InsertItem(&tvInsert);
		}
	}
}


void CDlgImportFootprint::OnBnClickedOk()
{
	// get state of tree control so we can reproduce it next time
	// get next top-level item
	HTREEITEM hItem = part_tree.GetNextItem( NULL, TVGN_CHILD );
	// get all items
	int ilib = -1;
	while( hItem )
	{
		// top-level item
		BOOL expanded = TVIS_EXPANDED & part_tree.GetItemState( hItem, TVIS_EXPANDED );
		CString str;
		if( ilib == -1 )
			gLocalCacheExpanded = expanded & TVIS_EXPANDED;
		else
			m_footlibfolder->SetExpanded( ilib, expanded & TVIS_EXPANDED );
		// get next top-level item
		hItem = part_tree.GetNextItem( hItem, TVGN_NEXT );
		ilib++;
	}
	// save filename and folder of footprint to be imported
	gLastFileName = m_footprint_filename;
	gLastFolderName = m_footprint_folder;
	OnOK();
}

void CDlgImportFootprint::OnBnClickedCancel()
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
			m_footlibfolder->SetExpanded( ilib, expanded & TVIS_EXPANDED );
		// get next top-level item
		item = part_tree.GetNextItem( item, TVGN_NEXT );
		ilib++;
	}
	OnCancel();
}
