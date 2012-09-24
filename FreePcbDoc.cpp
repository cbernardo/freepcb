// FreePcbDoc.cpp : implementation of the CFreePcbDoc class
//
#pragma once

#include "stdafx.h"
#include <direct.h>
#include <shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "PcbFont.h"
#include "DlgAddPart.h"
#include "DlgEditNet.h"
#include "DlgAssignNet.h"
#include "DlgNetlist.h"
#include "DlgProjectOptions.h"
#include "DlgImportOptions.h" 
#include "DlgLayers.h"
#include "DlgPartlist.h"
#include "MyFileDialog.h"
#include "MyFileDialogExport.h" 
#include "DlgIvex.h"
#include "DlgIndexing.h"
#include "DlgCAD.h"
#include "DlgWizQuad.h"
#include "utility.h"
#include "gerber.h"
#include "dlgdrc.h"
#include "DlgGroupPaste.h"
#include "DlgReassignLayers.h"
#include "DlgExportDsn.h"
#include "DlgImportSes.h"
#include "RTcall.h"
#include "DlgReport.h"
#include "DlgNetCombine.h"
#include "DlgMyMessageBox.h"
#include "DlgSaveLib.h"
#include "DlgPrefs.h"
#include "DlgGridVals.h"
#include "DlgAddGridVal.h"
#include "DlgExportOptions.h"	// CPT
#include "Part.h"
#include "Text.h"
#include <Shtypes.h>
#include <Shobjidl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFreePcbApp theApp;
CFreePcbDoc * this_Doc;		// global for callback

// global array to map file_layers to actual layers
int m_layer_by_file_layer[MAX_LAYERS];


/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc

IMPLEMENT_DYNCREATE(CFreePcbDoc, CDocument)

BEGIN_MESSAGE_MAP(CFreePcbDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_VIEW_NETLIST, OnProjectNetlist)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_COMMAND(ID_VIEW_LAYERS, OnViewLayers)
	ON_COMMAND(ID_VIEW_PARTLIST, OnProjectPartlist)
	ON_COMMAND(ID_FILE_IMPORT, OnFileImport)
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_COMMAND(ID_FILE_CONVERT, OnFileConvert)
	ON_COMMAND(ID_FILE_GENERATECADFILES, OnFileGenerateCadFiles)
	ON_COMMAND(ID_TOOLS_FOOTPRINTWIZARD, OnToolsFootprintwizard)
	ON_COMMAND(ID_PROJECT_OPTIONS, OnProjectOptions)
	ON_COMMAND(ID_FILE_EXPORTNETLIST, OnFileExport)
	ON_COMMAND(ID_TOOLS_CHECK_PARTS_NETS, OnToolsCheckPartsAndNets)
	ON_COMMAND(ID_TOOLS_CHECK_CONNECTIVITY, OnToolsCheckConnectivity)
	ON_COMMAND(ID_VIEW_LOG, OnViewLog)
	ON_COMMAND(ID_TOOLS_CHECKCOPPERAREAS, OnToolsCheckCopperAreas)
	ON_COMMAND(ID_TOOLS_CHECKTRACES, OnToolsCheckTraces)
	ON_COMMAND(ID_EDIT_PASTEFROMFILE, OnEditPasteFromFile)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_DSN_FILE_EXPORT, OnFileExportDsn)
	ON_COMMAND(ID_SES_FILE_IMPORT, OnFileImportSes)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_FILE_GENERATEREPORTFILE, OnFileGenerateReportFile)
	ON_COMMAND(ID_FILE_LOADLIBRARYASPROJECT, OnFileLoadLibrary)
	ON_COMMAND(ID_FILE_SAVEPROJECTASLIBRARY, OnFileSaveLibrary)
	// CPT
	ON_COMMAND(ID_TOOLS_PREFERENCES, OnToolsPreferences)
	ON_COMMAND(ID_VIEW_ROUTINGGRIDVALUES, OnViewRoutingGrid)
	ON_COMMAND(ID_VIEW_PLACEMENTGRIDVALUES, OnViewPlacementGrid)
	ON_COMMAND(ID_VIEW_VISIBLEGRIDVALUES, OnViewVisibleGrid)
	// end CPT
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc construction/destruction

CFreePcbDoc::CFreePcbDoc()
{
	// get application directory.  (CPT2 altered the method)
	TCHAR szPath[MAX_PATH];
	GetModuleFileName( NULL, szPath, MAX_PATH ); 
	CString exePath = szPath;
	int pos = exePath.ReverseFind('\\');
	m_app_dir = exePath.Left( pos );
	m_app_dir.Trim();
	int err = _chdir( m_app_dir );	// change to application folder
	if( err )
		ASSERT(0);	// failed to switch to application folder

	m_smfontutil = new SMFontUtil( &m_app_dir );
	m_pcbu_per_wu = 25400;	// default nm per world unit
	DWORD dwVersion = ::GetVersion();
	m_WindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if( m_WindowsMajorVersion > 4 )
		m_pcbu_per_wu = 2540;		// if Win2000 or XP or vista
	m_dlist = m_dlist_pcb = new CDisplayList( m_pcbu_per_wu, m_smfontutil );
	m_dlist_fp = new CDisplayList( m_pcbu_per_wu, m_smfontutil );
	m_plist = new CPartList( this );
	m_nlist = new CNetList( this );
	m_plist->UseNetList( m_nlist );
	m_tlist = new CTextList( this );
	m_slist = new CShapeList( this );
	m_drelist = new CDreList( this );
	m_pcb_filename = "";
	m_pcb_full_path = "";
	m_project_open = FALSE;
	m_project_modified = FALSE;
	m_project_modified_since_autosave = FALSE;
	m_footprint_modified = FALSE;
	m_footprint_name_changed = FALSE;
	theApp.m_doc = this;
	this_Doc = this;
	m_auto_interval = 0;
	m_auto_elapsed = 0;
	m_dlg_log = NULL;
	bNoFilesOpened = TRUE;
	m_version = 1.400;													// CPT2 TODO discuss...
	m_file_version = 1.344;
	m_dlg_log = new CDlgLog;
	m_dlg_log->Create( IDD_LOG );
	m_import_flags = IMPORT_PARTS | IMPORT_NETS | KEEP_FP | KEEP_NETS;
	m_num_copper_layers = 1;
	m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
	m_snap_angle = m_fp_snap_angle = 45;								// CPT2 bugfix
	m_edit_footprint = NULL;											// CPT2

	// initialize pseudo-clipboard
	clip_plist = new CPartList( this );
	clip_nlist = new CNetList( this );
	clip_plist->UseNetList( clip_nlist );
	clip_tlist = new CTextList( this );
	clip_slist = new CShapeList( this );
}

CFreePcbDoc::~CFreePcbDoc()
{
	// CPT2 TODO I think this is only invoked when program is shutting down (check), so it's not worth putting a lot of effort into it...
	// delete partlist, netlist, displaylist, etc.
	delete m_drelist;
	delete m_nlist;
	delete m_plist;
	delete m_tlist;
	delete m_slist;
	delete m_dlist;
	delete m_dlist_fp;
	delete m_smfontutil;
	boards.DestroyAll();
	smcutouts.DestroyAll();
	// Delete clipboard objects
	delete clip_nlist;
	delete clip_plist;
	delete clip_tlist;
	delete clip_slist;		//** AMW3
	clip_boards.DestroyAll();
	clip_smcutouts.DestroyAll();

	// delete log window
	if( m_dlg_log )
	{
		m_dlg_log->DestroyWindow();
		delete m_dlg_log;
	}
}

void CFreePcbDoc::SendInitialUpdate()
{
	CDocument::SendInitialUpdate();
}

// this is only executed once, at beginning of app
//
BOOL CFreePcbDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// CPT
	CString s1 ((LPCSTR) IDS_AppName), s2 ((LPCSTR) IDS_NoProjectOpen);
	m_window_title = s1 + " - " + s2;
	// end CPT

	m_parent_folder = "..\\projects\\";
	m_lib_dir = "..\\lib\\" ;
	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc serialization

void CFreePcbDoc::Serialize(CArchive& ar)
{
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc diagnostics

#ifdef _DEBUG
void CFreePcbDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFreePcbDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc commands


BOOL CFreePcbDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	return CDocument::OnSaveDocument(lpszPathName);
}

// CPT: helper routines defined further below:
void ReadFileLines(CString &fname, CArray<CString> &lines); 
void WriteFileLines(CString &fname, CArray<CString> &lines);
void ReplaceLines(CArray<CString> &oldLines, CArray<CString> &newLines, char *key);
// end CPT

void CFreePcbDoc::OnFileNew()
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_view_fp->OnFootprintFileNew();
		return;
	}

	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();

	// now set default project options
	InitializeNewProject();
	CDlgProjectOptions dlg;
	// CPT: args have changed.  Including m_path_to_folder instead of m_parent_folder (don't really understand the point of m_parent_folder anyway).
	//** AMW3 restored original behavior, which I prefer
	//** dlg.Init( TRUE, &m_name, &m_path_to_folder, &m_lib_dir, m_bSyncFile, &m_sync_file,
	dlg.Init( TRUE, &m_name, &m_parent_folder, &m_lib_dir, m_bSyncFile, &m_sync_file,
		m_num_copper_layers, m_bSMT_copper_connect, m_default_glue_w,
		m_trace_w, m_via_w, m_via_hole_w,
		&m_w, &m_v_w, &m_v_h_w );
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	// set up project file name and path
	m_name = dlg.GetName();
	m_pcb_filename = m_name + ".fpc";
	CString fullpath;
	char full[_MAX_PATH];
	fullpath = _fullpath( full, (LPCSTR)dlg.GetPathToFolder(), MAX_PATH );
	m_path_to_folder = (CString)fullpath;

	// Check if project folder exists and create it if necessary
	struct _stat buf;
	int err = _stat( m_path_to_folder, &buf );
	if( err )
	{
		CString str, s ((LPCSTR) IDS_FolderDoesntExistCreateIt);
		str.Format( s, m_path_to_folder );
		int ret = AfxMessageBox( str, MB_YESNO );
		if( ret == IDYES )
		{
			err = _mkdir( m_path_to_folder );
			if( err )
			{
				CString s ((LPCSTR) IDS_UnableToCreateFolder);
				str.Format( s, m_path_to_folder );
				AfxMessageBox( str, MB_OK );
			}
		}
	}
	if( err )
		return;

	CString str;
	m_pcb_full_path = (CString)fullpath	+ "\\" + m_pcb_filename;
	CString s1 ((LPCSTR) IDS_AppName);
	m_window_title = s1 + " - " + m_pcb_filename;
	CWnd* pMain = AfxGetMainWnd();
	pMain->SetWindowText( m_window_title );

	// make path to library folder and index libraries
	m_lib_dir = dlg.GetLibFolder();
	fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
	if( fullpath[fullpath.GetLength()-1] == '\\' )	
		fullpath = fullpath.Left(fullpath.GetLength()-1);
	m_full_lib_dir = fullpath;
	MakeLibraryMaps( &m_full_lib_dir );
	CMenu* pMenu = pMain->GetMenu();
	pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_ENABLED ); 
	pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_ENABLED ); 
	pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_ENABLED ); 
	pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_ENABLED ); 
	pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_ENABLED ); 
	CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
	submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED );	
	submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED );	
	submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED );	
	submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
	submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_ENABLED );	
	submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_ENABLED );	
	submenu->EnableMenuItem( ID_FILE_GENERATEREPORTFILE, MF_BYCOMMAND | MF_ENABLED );	
	submenu->EnableMenuItem( ID_DSN_FILE_EXPORT, MF_BYCOMMAND | MF_ENABLED );	
	submenu->EnableMenuItem( ID_SES_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
	submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
	// CPT: Eliminated 2 lines:
	// submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	// submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	pMain->DrawMenuBar();

	// set options from dialog
	m_num_copper_layers = dlg.GetNumCopperLayers();
	m_nlist->SetSMTconnect( m_bSMT_copper_connect );
	m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
	m_trace_w = dlg.GetTraceWidth();
	m_via_w = dlg.GetViaWidth();
	m_via_hole_w = dlg.GetViaHoleWidth();
	m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
	for( int i=0; i<m_num_layers; i++ )
	{
		m_vis[i] = 1;
		m_dlist->SetLayerRGB( i, C_RGB( m_rgb[i][0],m_rgb[i][1],m_rgb[i][2] ) );
	}
	// CPT2 new
	m_bSyncFile = dlg.m_bSyncFile;
	m_sync_file = dlg.m_sync_file;

	// CPT: option to save dlg results to default.cfg:
	if (dlg.m_default) 
	{
		CArray<CString> oldLines, newLines;
		CString fn = m_defaultcfg_dir + "\\" + "default.cfg";
		ReadFileLines(fn, oldLines);
		CollectOptionsStrings(newLines);
		ReplaceLines(oldLines, newLines, "path_to_folder");
		ReplaceLines(oldLines, newLines, "library_folder");
		ReplaceLines(oldLines, newLines, "n_copper_layers");
		ReplaceLines(oldLines, newLines, "SMT_connect_copper");
		ReplaceLines(oldLines, newLines, "default_glue_width");
		ReplaceLines(oldLines, newLines, "default_trace_width");
		ReplaceLines(oldLines, newLines, "default_via_pad_width");
		ReplaceLines(oldLines, newLines, "default_via_hole_width");
		ReplaceLines(oldLines, newLines, "n_width_menu");
		ReplaceLines(oldLines, newLines, "width_menu_item");
		WriteFileLines(fn, oldLines);
	}

	// CPT:  fixed bug where m_view->m_dlist->SetMapping() didn't get called, with unpredictable results for the initial
	// display
	m_project_open = TRUE;
	m_view->OnViewAllElements();

	// force redraw of function key text
	m_view->m_cursor_mode = 999;
	m_view->SetCursorMode( CUR_NONE_SELECTED );

	// force redraw of window title
	m_project_modified = FALSE;
	m_project_modified_since_autosave = FALSE;
	m_auto_elapsed = 0;

	// save project
	OnFileSave();
#ifdef CPT2_LOG
	// CPT2 logging feature:  copy the file to a new log file, whose name is fn + ".log".
	CString logFn = CString(m_pcb_full_path) + ".log";
	CopyFile(m_pcb_full_path, logFn, false);
	CString line;
	line.Format( "@@@\r\nu %d\r\n", CPcbItem::next_uid);
	m_view->Log(line, false);
#endif
	
	// CPT2 new:
	if (m_bSyncFile)
	{
		CString s ((LPCSTR) IDS_DoYouWishToImportFromTheNetlistFileNow);
		int ret = AfxMessageBox( s, MB_YESNO );
		if (ret==IDYES)
			FileImport(m_sync_file, true);
	}
}

// I created the following routine while struggling to understand the misbehavior of the CFileDialog interface.  (Turns out the solution is
// just to build in release mode rather than debug mode --- debug object code and common control routines apparently conflict pretty badly.)
// Anyway, during the process I figured out (with considerable difficulty) how to use the new Vista-style common controls.  Though it turns out not
// to be necessary, I'm leaving the code in as a comment, so that we can potentially use it one day...
/*
bool CFreePcbDoc::GetFileName(bool bSave, CString initial, int titleRsrc, int filterRsrc, WCHAR *defaultExt, WCHAR *result, int *offFileName)
{
	USES_CONVERSION;
	IFileDialog* pfod = 0;
	HRESULT hr = ::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfod));
	if (!SUCCEEDED(hr))
		return false;

    // New dialog starting with Vista/Windows 7
    // Set the file types to display.
    COMDLG_FILTERSPEC*  pOpenTypes = new COMDLG_FILTERSPEC[2];
    pOpenTypes[0].pszName = L"FreePCB Files";
    pOpenTypes[0].pszSpec = L"*.fpc";
	pOpenTypes[1].pszName = L"All files";
	pOpenTypes[1].pszSpec = L"*.*";
    hr = pfod->SetFileTypes(2, pOpenTypes);
    if (!SUCCEEDED(hr))
		{ pfod->Release(); return false; }
    hr = pfod->SetFileTypeIndex(0);
    if (!SUCCEEDED(hr))
		{ pfod->Release(); return false; }

    //  pfod->SetFileName(strFile);
    //  pfod->SetTitle(strTitle);

	// Ensure the dialog only returns file system paths, + other options flags.
	DWORD dwFlags;
	hr = pfod->GetOptions(&dwFlags);
    if (!SUCCEEDED(hr))
		{ pfod->Release(); return false; }
    dwFlags |= FOS_FORCEFILESYSTEM;
    // if(nFlags & OFN_FILEMUSTEXIST)
    //    dwFlags |= FOS_FILEMUSTEXIST;
    // if(nFlags & OFN_PATHMUSTEXIST)
    //    dwFlags |= FOS_PATHMUSTEXIST;
    hr = pfod->SetOptions(dwFlags);
    if (!SUCCEEDED(hr))
		{ pfod->Release(); return false; }
	hr = pfod->Show(0);
    if (!SUCCEEDED(hr))
		{ pfod->Release(); return false; }
    // Obtain the result once the user clicks the 'Open' button. The result is an IShellItem object.
    IShellItem *psiResult;
    hr = pfod->GetResult(&psiResult);
    if (!SUCCEEDED(hr))
		{ pfod->Release(); return false; }
	// Obtain the file-path from the shell-item.
    PWSTR pszFilePath = NULL;
	hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    if (!SUCCEEDED(hr))
		{ pfod->Release(); psiResult->Release(); return false; }

	// Copy pszFilePath into the return array/CString or whatever...

    CoTaskMemFree(pszFilePath);
    psiResult->Release();
    pfod->Release();
	return true;
}
*/

void CFreePcbDoc::OnFileOpen()
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_view_fp->OnFootprintFileImport();
		return;
	}

	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();
	InitializeNewProject();		// set defaults

	// get project file name
	CString s ((LPCSTR) IDS_PCBFiles);
	CFileDialog dlg( 1, "fpc", LPCTSTR(m_pcb_filename), 0, 
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );
	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_parent_folder;
	// now show dialog
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		CString pathname = dlg.GetPathName();
		if( pathname.Right(4) == ".fpl" )
		{
			CString mess ((LPCSTR) IDS_YouAreOpeningAFileWithExtensionFPL); 
			int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
			if( ret == IDCANCEL )
				return;
			else if( ret == IDYES )
				FileLoadLibrary( pathname );
		}
		else
			// read project file
			FileOpen( pathname );
	}
	else
	{
		// CANCEL or error
		DWORD dwError = ::CommDlgExtendedError();
		if( dwError )
		{
			CString str, s ((LPCSTR) IDS_FileOpenDialogErrorCode);
			str.Format( s, (unsigned long)dwError );
			AfxMessageBox( str );
		}
	}
}


void CFreePcbDoc::OnFileAutoOpen( LPCTSTR fn )
{
	CString pathname = fn;
	if( pathname.Right(4).MakeLower() == ".fpl" )
	{
		CString mess ((LPCSTR) IDS_YouAreOpeningAFileWithExtensionFPL);
		int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
		{
			FileLoadLibrary( fn );
			return;
		}
		else
			FileOpen( fn );
			return;
	}
	else
		FileOpen( fn );
}

// open project file, fn = full path to file
// if bLibrary = true, open library file and read footprints
// return TRUE if success
//
BOOL CFreePcbDoc::FileOpen( LPCTSTR fn, BOOL bLibrary )
{
	// if another file open, offer to save before closing
	if( FileClose() == IDCANCEL )
		return FALSE;		// file close cancelled
	
	// reset before opening new project
	m_view->CancelHighlight();
	m_view->CancelSelection();
	InitializeNewProject();		// set defaults

#ifdef CPT2_LOG
	// CPT2 logging feature:  copy the file to a new log file, whose name is fn + ".log".  (Unless we're performing the initiation of replay mode.)
	if (!m_view->m_bReplayMode)
	{
		CString logFn = CString(fn) + ".log";
		CopyFile(fn, logFn, false);
	}
#endif
	// now open it
	CStdioFile pcb_file;
	int err = pcb_file.Open( fn, CFile::modeRead, NULL );
	if( !err )
	{
		// error opening project file
		CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
		mess.Format( s, fn );
		AfxMessageBox( mess );
		return FALSE;
	}

	try
	{
		m_pcb_full_path = fn;
		int fpl = m_pcb_full_path.GetLength();
		int isep = m_pcb_full_path.ReverseFind( '\\' );
		if( isep == -1 )
			isep = m_pcb_full_path.ReverseFind( ':' );
		if( isep == -1 )
			ASSERT(0);		// unable to parse filename
		m_pcb_filename = m_pcb_full_path.Right( fpl - isep - 1);
		int fnl = m_pcb_filename.GetLength();
		m_path_to_folder = m_pcb_full_path.Left( m_pcb_full_path.GetLength() - fnl - 1 );
		m_project_open = TRUE;
		theApp.AddMRUFile( &m_pcb_full_path );

		if( !bLibrary )
		{
#ifdef CPT2_LOG
			if (!m_view->m_bReplayMode)
			{
				CString line;
				line.Format( "@@@\r\nu %d\r\n", CPcbItem::next_uid);
				m_view->Log(line, false);
			}
#endif

			CString key_str;
			CString in_str;
			CArray<CString> p;

			ReadOptions( &pcb_file );
			m_slist->ReadShapes( &pcb_file );
			ReadBoardOutline( &pcb_file );
			ReadSolderMaskCutouts( &pcb_file );
			m_plist->ReadParts( &pcb_file );
			m_nlist->ReadNets( &pcb_file, m_read_version );
			m_tlist->ReadTexts( &pcb_file );
			m_plist->SetThermals();
			Redraw();											// pcb is loaded, so go ahead and draw the items added to this->redraw during the prior 6 lines

			// CPT2 new: Load the synched netlist file info if that option is set
			if (m_bSyncFile)
				LoadSyncFile();

			// make path to library folder and index libraries
			if( m_full_lib_dir == "" )
			{
				CString fullpath;
				char full[MAX_PATH];
				fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
				if( fullpath[fullpath.GetLength()-1] == '\\' )	
					fullpath = fullpath.Left(fullpath.GetLength()-1);
				m_full_lib_dir = fullpath;
			}
			MakeLibraryMaps( &m_full_lib_dir );
		}
		else
			// read library as project
			m_slist->ReadShapes( &pcb_file, false );
		CString s1 ((LPCSTR) IDS_AppName);
		m_window_title = s1 + " - " + m_pcb_filename;
		CWnd* pMain = AfxGetMainWnd();
		pMain->SetWindowText( m_window_title );
		if( m_name == "" )
		{
			m_name = m_pcb_filename;
			if( m_name.Right(4) == ".fpc" )
				m_name = m_name.Left( m_name.GetLength() - 4 );
		}
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_ENABLED ); 
			CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
			submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_GENERATEREPORTFILE, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_DSN_FILE_EXPORT, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_SES_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
			submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			// CPT: eliminated 2 lines:
			// submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			// submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			pMain->DrawMenuBar();
		}
		// now set layer visibility
		for( int i=0; i<m_num_layers; i++ )
		{
			m_dlist->SetLayerRGB( i, C_RGB(m_rgb[i][0],m_rgb[i][1],m_rgb[i][2]) );
			m_dlist->SetLayerVisible( i, m_vis[i] );
		}
		// force redraw of function key text
		m_view->m_cursor_mode = 999;
		m_view->SetCursorMode( CUR_NONE_SELECTED );
		ProjectModified( FALSE );
		m_view->OnViewAllElements();
		m_auto_elapsed = 0;
		m_plist->CheckForProblemFootprints();
		bNoFilesOpened = FALSE;
		ResetUndoState();						// CPT2 --- important under the new system to do this AFTER all the new objects are loaded.
		return TRUE;
	}
	catch( CString * err_str )
	{
		// parsing error
		AfxMessageBox( *err_str );
		delete err_str;
		CDC * pDC = m_view->GetDC();
		m_view->OnDraw( pDC );
		m_view->ReleaseDC( pDC );
		redraw.RemoveAll();						// CPT2 bug fix.  Clear out stuff from the corrupt file that was slated for drawing
		m_project_open = false;
		return FALSE;
	}
}

void CFreePcbDoc::LoadSyncFile()
{
	CStdioFile file;
	if( !file.Open( m_sync_file, CFile::modeRead ) )
	{
		CString s0 ((LPCSTR) IDS_UnableToOpenTheReferencedNetlistFile), mess;
		mess.Format(s0, m_sync_file);
		AfxMessageBox( mess );
		m_bSyncFile = false;
		return;
	}
	m_import_flags = IMPORT_FROM_NETLIST_FILE | IMPORT_PARTS | IMPORT_NETS | DONT_LOAD_SHAPES;
	// CPT2 TODO assuming for the time being a PADS-PCB format.
	int err = ImportPADSPCBNetlist( &file, m_import_flags, &m_sync_file_pli, &m_sync_file_nli );
	if( err == NOT_PADSPCB_FILE )
	{
		m_dlg_log->ShowWindow( SW_HIDE );
		CString s0 ((LPCSTR) IDS_WarningTheReferencedNetlistFileDoesNotAppearToBeALegalPADSPCB), mess;
		mess.Format(s0, m_sync_file);
		AfxMessageBox( mess, MB_OK );
		m_bSyncFile = false;
	}
}

void CFreePcbDoc::OnFileClose()
{
	FileClose();
	m_view->m_bReplayMode = false;
}

// return IDCANCEL if closing cancelled by user
//
int CFreePcbDoc::FileClose()
{
	if( m_project_open && m_project_modified && !m_view->m_bReplayMode)
	{
		CString s ((LPCSTR) IDS_ProjectModifiedSaveIt);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return IDCANCEL;
		else if( ret == IDYES )
			OnFileSave();
	}
	else if (m_view->m_bReplayMode && m_view->m_log_file.m_pStream)
		m_view->m_log_file.Close();
	m_view->CancelSelection();

	// destroy existing project
	// delete undo list, partlist, netlist, displaylist, etc.
	ResetUndoState();
	m_nlist->nets.RemoveAll();
	m_plist->parts.RemoveAll();
	m_tlist->texts.RemoveAll();
	m_slist->shapes.RemoveAll();
	smcutouts.RemoveAll();
	boards.RemoveAll();
	m_drelist->dres.RemoveAll();
	m_dlist->RemoveAll();
	// clear clipboard
	clip_nlist->nets.RemoveAll();
	clip_plist->parts.RemoveAll();
	clip_tlist->texts.RemoveAll();
	clip_smcutouts.RemoveAll();
	clip_boards.RemoveAll();
	GarbageCollect();	//** AMW3

	CWnd* pMain = AfxGetMainWnd();
	if (pMain != NULL)
	{
		CMenu* pMenu = pMain->GetMenu();
		pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
		submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_GENERATEREPORTFILE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_DSN_FILE_EXPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_SES_FILE_IMPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
		// CPT: eliminated 2 lines:
		// submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		// submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		pMain->DrawMenuBar();
	}

	m_view->Invalidate( FALSE );
	m_project_open = FALSE;
	ProjectModified( FALSE );
	m_auto_elapsed = 0;
	m_bSyncFile = false;			// CPT2 new
	m_sync_file = "";				// ditto
	// force redraw
	m_view->m_cursor_mode = 999;
	m_view->SetCursorMode( CUR_NONE_SELECTED );
	CString s1 ((LPCSTR) IDS_AppName), s2 ((LPCSTR) IDS_NoProjectOpen);
	m_window_title = s1 + " - " + s2;
	pMain->SetWindowText( m_window_title );
	CDC * pDC = m_view->GetDC();
	m_view->OnDraw( pDC );
	m_view->ReleaseDC( pDC );
	return IDOK;
}

void CFreePcbDoc::OnFileSave() 
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_view_fp->OnFootprintFileSaveAs();
		return;
	}

	PurgeFootprintCache();
	FileSave( &m_path_to_folder, &m_pcb_filename, &m_path_to_folder, &m_pcb_filename );
	ProjectModified( FALSE );
	ResetUndoState();
	bNoFilesOpened = FALSE;
}

// Autosave
//
BOOL CFreePcbDoc::AutoSave()
{
	CString str;
	CString auto_folder = m_path_to_folder + "\\Autosave";
	int len = m_pcb_filename.GetLength();
	CString search_str = m_pcb_filename + ".0??";
	struct _stat s;
	int err = _stat( auto_folder, &s );
	if( err )
	{
		if( err )
		{
			err = _mkdir( auto_folder );
			if( err )
			{
				CString s ((LPCSTR) IDS_UnableToCreateAutosaveFolder);
				str.Format( s, auto_folder );
				AfxMessageBox( str, MB_OK );
				return FALSE;
			}
		}
	}
	CFileFind finder;
	CString list_str = "";
	CTime time;
	time_t bin_time;
	time_t max_time = 0;
	int max_suffix = 0;
	if( _chdir( auto_folder ) != 0 )
	{
		CString mess, s ((LPCSTR) IDS_UnableToOpenAutosaveFolder);
		mess.Format( s, auto_folder );
		AfxMessageBox( mess );
	}
	else
	{
		BOOL bWorking = finder.FindFile( search_str );
		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			CString fn = finder.GetFileName();
			if( !finder.IsDirectory() )
			{
				// found a file
				int suffix = atoi( fn.Right(2) );
				finder.GetLastWriteTime( time );
				bin_time = time.GetTime();
				if( bin_time > max_time )
				{
					max_time = bin_time;
					max_suffix = suffix;
				}
			}
		}
	}
	CString new_file_name;
	int new_suffix = max_suffix%25 + 1;
	new_file_name.Format( "%s.%03d", m_pcb_filename, new_suffix );
	FileSave( &auto_folder, &new_file_name, NULL, NULL, FALSE );
	m_project_modified_since_autosave = FALSE;
	finder.Close();
	return TRUE;
}

// save project file
// make backup if the new file has the same path and filename as the old file
// returns TRUE if successful, FALSE if fails
//
BOOL CFreePcbDoc::FileSave( CString * folder, CString * filename, 
						   CString * old_folder, CString * old_filename,
						   BOOL bBackup ) 
{
	if( !m_project_open )
		return FALSE;

	// write project file
	CString full_path = *folder + "\\" + *filename;
	// see if we need to make a backup file
	if( old_folder != NULL && old_filename != NULL )
	{
		if( bBackup && *folder == *old_folder && *filename == *old_filename )
		{
			// rename file
			CString backup = full_path + ".bak";
			remove( backup );
			rename( full_path, backup );
		}
	}
	CStdioFile pcb_file;
	if (!pcb_file.Open( LPCSTR(full_path), CFile::modeCreate | CFile::modeWrite, NULL ))
	{
		// error opening file
		CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
		mess.Format( s, full_path ); 
		AfxMessageBox( mess );
		return FALSE;
	} 
	// write project to file
	try
	{
		WriteOptions( &pcb_file );
		m_slist->WriteShapes( &pcb_file );
		WriteBoardOutline( &pcb_file );
		WriteSolderMaskCutouts( &pcb_file );
		m_plist->WriteParts( &pcb_file );
		m_nlist->WriteNets( &pcb_file );
		m_tlist->WriteTexts( &pcb_file );
		pcb_file.WriteString( "[end]\n" );
		pcb_file.Close();
		theApp.AddMRUFile( &m_pcb_full_path );
		bNoFilesOpened = FALSE;
		m_auto_elapsed = 0;
	}
	catch( CString * err_str )
	{
		// error
		AfxMessageBox( *err_str );
		delete err_str;
		CDC * pDC = m_view->GetDC();
		m_view->OnDraw( pDC ) ;
		m_view->ReleaseDC( pDC );
		return FALSE;
	}

	return TRUE;
}

void CFreePcbDoc::OnFileSaveAs() 
{
	CString s ((LPCSTR) IDS_PCBFiles);
	CFileDialog dlg( 0, "fpc", LPCTSTR(m_pcb_filename), OFN_OVERWRITEPROMPT,							// CPT changed arg
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );	
//	OPENFILENAME  * myOFN = dlg.m_pOFN;
//	myOFN->Flags |= OFN_OVERWRITEPROMPT;																// CPT: this way of setting options was causing 
																										// doModal() to fail if file had changed
	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_parent_folder;
	int err = dlg.DoModal();
	if( err != IDOK )
		return;

	// get new filename and folder
	CString new_pathname = dlg.GetPathName();
	CString new_filename = dlg.GetFileName();
	int fnl = new_filename.GetLength();
	CString new_folder = new_pathname.Left( new_pathname.GetLength() - fnl - 1 );
	// write project file
	BOOL ok = FileSave( &new_folder, &new_filename, &m_path_to_folder, &m_pcb_filename );
	if( ok )
	{
		// update member variables, MRU files and window title
		m_pcb_filename = new_filename;
		m_pcb_full_path = new_pathname;
		m_path_to_folder = new_folder;
		theApp.AddMRUFile( &m_pcb_full_path );
		CString s1 ((LPCSTR) IDS_AppName);
		m_window_title = s1 + " - " + m_pcb_filename;
		CWnd* pMain = AfxGetMainWnd();
		pMain->SetWindowText( m_window_title );
		ProjectModified( FALSE );
	}
	else
	{
		CString s ((LPCSTR) IDS_FileSaveFailed);
		AfxMessageBox( s );
	}
}

void CFreePcbDoc::OnProjectNetlist()
{
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	CDlgNetlist dlg;
	dlg.Initialize( m_nlist, m_plist, &m_w, &m_v_w, &m_v_h_w );
	if (!view->m_bReplaying)
	{
		int ret = dlg.DoModal();
		if( ret != IDOK )
			{ view->LogCancel(); return; }
	}
	view->Log(LOG_ON_PROJECT_NETLIST);
	view->LogNetListInfo( dlg.m_nli );

	// CPT2 new:  save undo info --- there's quite a lot of it
	CIter<CNet> in (&m_nlist->nets);
	for (CNet *net = in.First(); net; net = in.Next())
		net->SaveUndoInfo();																// NB this saves pin info too...
	m_nlist->ImportNetListInfo( dlg.m_nli, 0, NULL, m_trace_w, m_via_w, m_via_hole_w );
	// CPT2 Go through the modified netlist, and add pins to nets according to the synched netlist file where applicable
	if (m_bSyncFile)
		for (CNet *net = in.First(); net; net = in.Next())
			net->AddPinsFromSyncFile();
	if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
		m_nlist->OptimizeConnections();
	view->CancelSelection();
	ProjectModified( TRUE );
}


// get shape from cache
// if necessary, make shape from library file and put into cache first
// returns NULL if shape not found
//
CShape * CFreePcbDoc::GetFootprintPtr( CString *name )
{
	// lookup shape, first in cache
	CShape *s = m_slist->GetShapeByName( name );
	if (s)
		return s;
	// not in cache, lookup in library file
	int ilib;
	CString file_name;
	int offset;
	CString * project_lib_folder_str = m_footlibfoldermap.GetDefaultFolder();
	CFootLibFolder * project_footlibfolder = m_footlibfoldermap.GetFolder( project_lib_folder_str, m_dlg_log );
	BOOL ok = project_footlibfolder->GetFootprintInfo( name, NULL, &ilib, NULL, &file_name, &offset );
	if( !ok )
		// unable to find shape
		return NULL;

	// make shape from library file and put into cache
	CShape * shape = new CShape(this);
	CString lib_name = *project_footlibfolder->GetFullPath();
	int err = shape->MakeFromFile( NULL, *name, file_name, offset ); 
	if( err )
	{
		// failed
		CString mess, s ((LPCSTR) IDS_UnableToMakeShapeFromFile);
		mess.Format( s, name );
		AfxMessageBox( mess );
		return NULL;
	}
	// success, put into cache and return pointer
	m_slist->shapes.Add( shape );
	ProjectModified( TRUE );
	return shape;
}

// write board outline to file
//
// throws CString * exception on error
//
void CFreePcbDoc::WriteBoardOutline( CStdioFile * file, CHeap<CBoard> *boards0 )
{
	// CPT2 converted
	try
	{
		if (!boards0) 
			boards0 = &this->boards;
		CString line;
		line.Format( "[board]\n" );
		file->WriteString( line );
		CIter<CBoard> ib (boards0);
		int i = 1;
		for (CBoard *b = ib.First(); b; b = ib.Next())
		{
			line.Format( "\noutline: %d %d\n", b->NumCorners(), i++ );
			file->WriteString( line );
			CCorner *c = b->main->head;
			int icor = 1;
			do 
			{
				line.Format( "  corner: %d %d %d %d\n", icor++, c->x, c->y, c->postSide->m_style);
				file->WriteString( line );
				c = c->postSide->postCorner;
			}
			while (c!=b->main->head);
		}
		file->WriteString( "\n" );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteBoardOutline()\n" + *err_str;
		throw err_str;
	}
}
void CFreePcbDoc::WriteSolderMaskCutouts( CStdioFile * file, CHeap<CSmCutout> *smcutouts0 )
{
	// CPT2 TODO converted.  NB note that corners now need to be output with the endcontour bit indicated.  Does this imply the need for a new
	// file-version number?
	try
	{
		if (!smcutouts0)
			smcutouts0 = &smcutouts;
		CString line;
		line.Format( "[solder_mask_cutouts]\n\n" );
		file->WriteString( line );
		CIter<CSmCutout> ism (smcutouts0);
		for (CSmCutout *sm = ism.First(); sm; sm = ism.Next())
		{
			line.Format( "sm_cutout: %d %d %d\n", sm->NumCorners(), sm->m_hatch, sm->m_layer );
			file->WriteString( line );
			int icor = 1;
			CIter<CContour> ictr (&sm->contours);
			for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
			{
				CCorner *c = ctr->head;
				line.Format( "  corner: %d %d %d %d\n", icor++, c->x, c->y, c->postSide->m_style);
				file->WriteString(line);
				for (c = c->postSide->postCorner; c!=ctr->head; c = c->postSide->postCorner)
				{
					line.Format( "  corner: %d %d %d %d %d\n", icor++, c->x, c->y, 
						c->postSide->m_style, c->postSide->postCorner==ctr->head);
					file->WriteString(line);
				}
			} 
		}
		file->WriteString( "\n" );
	}

	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteSolderMaskCutouts()\n" + *err_str;
		throw err_str;
	}
}

// read board outline from file
// throws CString * exception on error
//
void CFreePcbDoc::ReadBoardOutline( CStdioFile * pcb_file )
{
	CString in_str, key_str;
	CArray<CString> p;
	int last_side_style = CPolyline::STRAIGHT;

	try
	{
		// find beginning of [board] section
		do
		{
			if (!pcb_file->ReadString( in_str ))
			{
				CString mess ((LPCSTR) IDS_UnableToFindBoardSectionInFile);
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str != "[board]" );

		// get data
		while( 1 )
		{
			int pos = pcb_file->GetPosition();
			if (!pcb_file->ReadString( in_str ))
				throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				return;
			}
			int np = ParseKeyString( &in_str, &key_str, &p );
			if( !np || key_str != "outline" )
				continue;
			if( np != 2 && np != 3 )
				throw new CString((LPCSTR) IDS_ErrorParsingBoardSectionOfProjectFile);
			int ncorners = my_atoi( &p[0] );
			int ib = 0;
			if( np == 3 )
				ib = my_atoi( &p[1] );
			CBoard *b = new CBoard(this);
			CContour *ctr = new CContour(b, true);				// Adds ctr as b's main contour
			for( int icor=0; icor<ncorners; icor++ )
			{
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "corner" || np < 4 )
					throw new CString((LPCSTR) IDS_ErrorParsingBoardSectionOfProjectFile);
				int ncor = my_atoi( &p[0] );
				if( (ncor-1) != icor )
					throw new CString((LPCSTR) IDS_ErrorParsingBoardSectionOfProjectFile);
				int x = my_atoi( &p[1] );
				int y = my_atoi( &p[2] );
				bool bContourWasEmpty = ctr->corners.IsEmpty();
				CCorner *c = new CCorner(ctr, x, y);			// Constructor adds corner to ctr->corners (and may also set ctr->head/tail if
																// it was previously empty)
				if (!bContourWasEmpty)
				{
					CSide *s = new CSide(ctr, last_side_style);
					ctr->AppendSideAndCorner(s, c, ctr->tail);
				}
				last_side_style = np >= 5? my_atoi( &p[3] ): CPolyline::STRAIGHT;
				if( icor == (ncorners-1) )
					ctr->Close(last_side_style);
			}
			if (ncorners<3)
				boards.Remove(b);								// CPT2 a little sanity check, inspired by Dustin S.
			b->MustRedraw();
		}
	}

	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::ReadBoardOutline()\n" + *err_str;
		throw err_str;
	}
}

// read solder mask cutouts from file
//
// throws CString * exception on error
//
void CFreePcbDoc::ReadSolderMaskCutouts( CStdioFile * pcb_file )
{
	CString in_str, key_str;
	CArray<CString> p;
	int last_side_style = CPolyline::STRAIGHT;

	try
	{
		// find beginning of [solder_mask_cutouts] section
		//** AMW3 restored compatibility with version 1.2
		int pos = pcb_file->GetPosition();
		do
		{
			if (!pcb_file->ReadString( in_str ))
			{
				CString mess ((LPCSTR) IDS_UnableToFindSolderMaskCutoutsSectionInFile);
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str[0] != '[' );

		if( in_str != "[solder_mask_cutouts]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			return;
		}
		//** end AMW3

		// get data
		while( 1 )
		{
			int pos = pcb_file->GetPosition();
			if (!pcb_file->ReadString( in_str ))
				throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				return;
			}
			int np = ParseKeyString( &in_str, &key_str, &p );
			if (!np || key_str != "sm_cutout" )
				continue;
			if( np<4 )
				throw new CString((LPCSTR) IDS_ErrorParsingSolderMaskCutoutsSectionOfProjectFile);
			int ncorners = my_atoi( &p[0] );
			int hatch = my_atoi( &p[1] );
			int lay = my_atoi( &p[2] );
			CSmCutout *sm = new CSmCutout(this, lay, hatch);
			CContour *ctr = new CContour(sm, true);				// Adds ctr as b's main contour
			for( int icor=0; icor<ncorners; icor++ )
			{
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "corner" || np < 4 )
					throw new CString((LPCSTR) IDS_ErrorParsingSolderMaskCutoutsSectionOfProjectFile);
				int ncor = my_atoi( &p[0] );
				if( (ncor-1) != icor )
					throw new CString((LPCSTR) IDS_ErrorParsingSolderMaskCutoutsSectionOfProjectFile);
				int x = my_atoi( &p[1] );
				int y = my_atoi( &p[2] );
				bool bContourWasEmpty = ctr->corners.IsEmpty();
				CCorner *c = new CCorner(ctr, x, y);			// Constructor adds corner to ctr->corners (and may also set ctr->head/tail if
																// it was previously empty)
				if (!bContourWasEmpty)
				{
					CSide *s = new CSide(ctr, last_side_style);
					ctr->AppendSideAndCorner(s, c, ctr->tail);
				}
				last_side_style = np >= 5? my_atoi( &p[3] ): CPolyline::STRAIGHT;
				int end_cont = np >= 6? my_atoi( &p[4] ): 0;
				if( icor == (ncorners-1) || end_cont )
				{
					ctr->Close(last_side_style);
					if (icor<ncorners-1)
						ctr = new CContour(sm, false);			// Make a new secondary contour.  This is a CPT2 new option for sm-cutouts
				}
			}
			if (ncorners<3)
				smcutouts.Remove(sm);							// CPT2 a little sanity check inspired by Dustin S's problem.
			sm->MustRedraw();
		}
	}

	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::ReadSolderMaskCutouts()\n" + *err_str;
		throw err_str;
	}
}

// CPT
void AddGridVal(CArray<double> &arr, CArray<CString> &p, int np) {
	// CPT: helper for ReadOptions() below.  Reads a length value in mm or mils from a line in the options file that has been parsed into p.
	// Adds the resulting value (positive for mils, negative for mm) to "arr":
	CString str = np==3? p[1]: p[0];
	double value = my_atof( &str );
	if( str.Right(2) == "MM" || str.Right(2) == "mm" )
		arr.Add( -value );
	else
		arr.Add( value );
	}
// end CPT


// read project options from file
//
// throws CString * exception on error
//
void CFreePcbDoc::ReadOptions( CStdioFile * pcb_file )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;

	// initalize
	CFreePcbView * view = (CFreePcbView*)m_view;
	m_visible_grid.SetSize( 0 );
	m_visible_grid_hidden.SetSize( 0 );				// CPT
	m_part_grid.SetSize( 0 );
	m_part_grid_hidden.SetSize( 0 );
	m_routing_grid.SetSize( 0 );
	m_routing_grid_hidden.SetSize( 0 );
	m_fp_visible_grid.SetSize( 0 );
	m_fp_visible_grid_hidden.SetSize( 0 );
	m_fp_part_grid.SetSize( 0 );
	m_fp_part_grid_hidden.SetSize( 0 );
	m_name = "";
	m_dr.bCheckUnrouted = FALSE;
	for( int i=0; i<MAX_LAYERS; i++ )
		m_layer_by_file_layer[i] = i;
	m_bSMT_copper_connect = FALSE;
	m_default_glue_w = 25*NM_PER_MIL;
	m_report_flags = 0;

	try
	{
		// find beginning of [options] section
		do
		{
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				// error reading pcb file
				CString mess ((LPCSTR) IDS_UnableToFindOptionsSectionInFile);
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str != "[options]" );

		// get data
		while( 1 )
		{
			pos = pcb_file->GetPosition();
			if (!pcb_file->ReadString( in_str ))
				throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				break;
			}
			np = ParseKeyString( &in_str, &key_str, &p );
			if( np == 2 && key_str == "project_name" )
				m_name = p[0];
			else if( np == 2 && key_str == "version" )
				m_read_version = my_atof( &p[0] );
			else if( np == 2 && key_str == "file_version" )
			{
				double file_version = my_atof( &p[0] );
				if( file_version > m_version )
				{
					CString mess, s ((LPCSTR) IDS_WarningTheFileVersionIs);
					mess.Format( s, file_version, m_version );
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret == IDCANCEL )
					{
						CString * err_str = new CString((LPCSTR) IDS_ReadingProjectFileFailedCancelledByUser);
						throw err_str;
					}
				}
			}
			else if( np && key_str == "parent_folder" )
				m_parent_folder = p[0];
			// CPT.  The parent_folder vs path_to_folder distinction is unclear to me.
			else if( np && key_str == "path_to_folder" )
				m_path_to_folder = p[0];
			// CPT2 the following two are new:
			else if( np && key_str == "sync_file_on" )
				m_bSyncFile = my_atoi( &p[0] );
			else if( np && key_str == "sync_file" )
				m_sync_file = p[0];
			else if( np && key_str == "library_folder" )
				m_lib_dir = p[0];
			else if( np && key_str == "library_folder" )
				m_lib_dir = p[0];
			else if( np && key_str == "full_library_folder" )
				m_full_lib_dir = p[0];
			else if( np && key_str == "CAM_folder" )
				m_cam_full_path = p[0];
			else if( np && key_str == "netlist_file_path" )
				m_netlist_full_path = p[0];
			else if( np && key_str == "ses_file_path" )
				m_ses_full_path = p[0];
			else if( np && key_str == "dsn_flags" )
				m_dsn_flags = my_atoi( &p[0] );
			else if( np && key_str == "dsn_bounds_poly" )
				m_dsn_bounds_poly = my_atoi( &p[0] );
			else if( np && key_str == "dsn_signals_poly" )
				m_dsn_signals_poly = my_atoi( &p[0] );
			else if( np && key_str == "SMT_connect_copper" )
			{
				m_bSMT_copper_connect = my_atoi( &p[0] );
				m_nlist->SetSMTconnect( m_bSMT_copper_connect );
			}
			else if( np && key_str == "default_glue_width" )
				m_default_glue_w = my_atoi( &p[0] );
			else if( np && key_str == "n_copper_layers" )
			{
				m_num_copper_layers = my_atoi( &p[0] );
				m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
			}
			else if( np && key_str == "autosave_interval" )
				m_auto_interval = my_atoi( &p[0] );
			else if( np && key_str == "auto_ratline_disable" )
				m_auto_ratline_disable = my_atoi( &p[0] );
			else if( np && key_str == "auto_ratline_disable_min_pins" )
				m_auto_ratline_min_pins = my_atoi( &p[0] );
			else if( np && key_str == "netlist_import_flags" )
				m_import_flags = my_atoi( &p[0] );
			else if( np && key_str == "units" )
			{
				if( p[0] == "MM" )
					m_view->m_units = MM;
				else
					m_view->m_units = MIL;
			}

			// CPT: factored out shared code.  Allowed for "hidden" grid items. 
			else if( np && key_str == "visible_grid_spacing" )
				m_visual_grid_spacing = my_atof( &p[0] );
			else if( np && key_str == "visible_grid_item" )
				AddGridVal(m_visible_grid, p, np);
			else if( np && key_str == "visible_grid_hidden" )
				AddGridVal(m_visible_grid_hidden, p, np);

			else if( np && key_str == "placement_grid_spacing" )
				m_part_grid_spacing = my_atof( &p[0] );
			else if( np && key_str == "placement_grid_item" )
				AddGridVal(m_part_grid, p, np);
			else if( np && key_str == "placement_grid_hidden" )
				AddGridVal(m_part_grid_hidden, p, np);

			else if( np && key_str == "routing_grid_spacing" )
				m_routing_grid_spacing = my_atof( &p[0] );
			else if( np && key_str == "routing_grid_item" )
				AddGridVal(m_routing_grid, p, np);
			else if( np && key_str == "routing_grid_hidden" )
				AddGridVal(m_routing_grid_hidden, p, np);

			else if( np && key_str == "snap_angle" )
				m_snap_angle = my_atof( &p[0] );

			else if( np && key_str == "fp_visible_grid_spacing" )
				m_fp_visual_grid_spacing = my_atof( &p[0] );
			else if( np && key_str == "fp_visible_grid_item" )
				AddGridVal(m_fp_visible_grid, p, np);
			else if( np && key_str == "fp_visible_grid_hidden" )
				AddGridVal(m_fp_visible_grid_hidden, p, np);

			else if( np && key_str == "fp_placement_grid_spacing" )
				m_fp_part_grid_spacing = my_atof( &p[0] );
			else if( np && key_str == "fp_placement_grid_item" )
				AddGridVal(m_fp_part_grid, p, np);
			else if( np && key_str == "fp_placement_grid_hidden" )
				AddGridVal(m_fp_part_grid_hidden, p, np);

			else if( np && key_str == "fp_snap_angle" )
				m_fp_snap_angle = my_atof( &p[0] );
			// CAM stuff
			else if( np && key_str == "fill_clearance" )
				m_fill_clearance = my_atoi( &p[0] );
			else if( np && key_str == "mask_clearance" )
				m_mask_clearance = my_atoi( &p[0] );
			else if( np && key_str == "thermal_width" )
				m_thermal_width = my_atoi( &p[0] );
			else if( np && key_str == "min_silkscreen_width" )
				m_min_silkscreen_stroke_wid = my_atoi( &p[0] );
			else if( np && key_str == "pilot_diameter" )
				m_pilot_diameter = my_atoi( &p[0] );
			else if( np && key_str == "board_outline_width" )
				m_outline_width = my_atoi( &p[0] );
			else if( np && key_str == "hole_clearance" )
				m_hole_clearance = my_atoi( &p[0] );
			else if( np && key_str == "annular_ring_for_pins" )
				m_annular_ring_pins = my_atoi( &p[0] );
			else if( np && key_str == "annular_ring_for_vias" )
				m_annular_ring_vias = my_atoi( &p[0] );
			else if( np && key_str == "shrink_paste_mask" )
				m_paste_shrink = my_atoi( &p[0] );
			else if( np && key_str == "cam_flags" )
				m_cam_flags = my_atoi( &p[0] );
			else if( np && key_str == "cam_layers" )
				m_cam_layers = my_atoi( &p[0] );
			else if( np && key_str == "cam_drill_file" )
				m_cam_drill_file = my_atoi( &p[0] );
			else if( np && key_str == "cam_units" )
				m_cam_units = my_atoi( &p[0] );
			else if( np && key_str == "cam_n_x" )
				m_n_x = my_atoi( &p[0] );
			else if( np && key_str == "cam_n_y" )
				m_n_y = my_atoi( &p[0] );
			else if( np && key_str == "cam_space_x" )
				m_space_x = my_atoi( &p[0] );
			else if( np && key_str == "cam_space_y" )
				m_space_y = my_atoi( &p[0] );
			else if( np && key_str == "report_options" )
				m_report_flags = my_atoi( &p[0] );
			// DRC stuff
			else if( np && key_str == "drc_check_unrouted" )
				m_dr.bCheckUnrouted = my_atoi( &p[0] );
			else if( np && key_str == "drc_trace_width" )
				m_dr.trace_width = my_atoi( &p[0] );
			else if( np && key_str == "drc_pad_pad" )
				m_dr.pad_pad = my_atoi( &p[0] );
			else if( np && key_str == "drc_pad_trace" )
				m_dr.pad_trace = my_atoi( &p[0] );
			else if( np && key_str == "drc_trace_trace" )
				m_dr.trace_trace = my_atoi( &p[0] );
			else if( np && key_str == "drc_hole_copper" )
				m_dr.hole_copper = my_atoi( &p[0] );
			else if( np && key_str == "drc_annular_ring_pins" )
				m_dr.annular_ring_pins = my_atoi( &p[0] );
			else if( np && key_str == "drc_annular_ring_vias" )
				m_dr.annular_ring_vias = my_atoi( &p[0] );
			else if( np && key_str == "drc_board_edge_copper" )
				m_dr.board_edge_copper = my_atoi( &p[0] );
			else if( np && key_str == "drc_board_edge_hole" )
				m_dr.board_edge_hole = my_atoi( &p[0] );
			else if( np && key_str == "drc_hole_hole" )
				m_dr.hole_hole = my_atoi( &p[0] );
			else if( np && key_str == "drc_copper_copper" )
				m_dr.copper_copper = my_atoi( &p[0] );
			else if( np && key_str == "default_trace_width" )
			{
				m_trace_w = my_atoi( &p[0] );
				m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
			}
			else if( np && key_str == "default_via_pad_width" )
			{
				m_via_w = my_atoi( &p[0] );
				m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
			}
			else if( np && key_str == "default_via_hole_width" )
			{
				m_via_hole_w = my_atoi( &p[0] );
				m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
			}
			else if( np && key_str == "n_width_menu" )
			{
				int n = my_atoi( &p[0] );
				m_w.SetSize( n );
				m_v_w.SetSize( n );
				m_v_h_w.SetSize( n );
				for( int i=0; i<n; i++ )
				{
					pos = pcb_file->GetPosition();
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( np < 5 || key_str != "width_menu_item" )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingOptionsSectionOfProjectFile );
						throw err_str;
					}
					int ig = my_atoi( &p[0] ) - 1;
					if( ig != i )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingOptionsSectionOfProjectFile );
						throw err_str;
					}
					m_w[i] = my_atoi( &p[1] );
					m_v_w[i] = my_atoi( &p[2] );
					m_v_h_w[i] = my_atoi( &p[3] );
				}
			}
			else if( np && key_str == "layer_info" )
			{
				CString file_layer_name = p[0];
				int i = my_atoi( &p[1] ); 
				int layer = -1;
				for( int il=0; il<MAX_LAYERS; il++ )
				{
					CString layer_string = &layer_str[il][0];
					if( file_layer_name == layer_string )
					{
						SetFileLayerMap( i, il );
						layer = il;
						break;
					}
				}
				if( layer < 0 )
				{
					CString s ((LPCSTR) IDS_WarningLayerNotSupported), mess;
					mess.Format(s, file_layer_name);
					AfxMessageBox( mess );
				}
				else
				{
					m_rgb[layer][0] = my_atoi( &p[2] );
					m_rgb[layer][1] = my_atoi( &p[3] );
					m_rgb[layer][2] = my_atoi( &p[4] );
					m_vis[layer] = my_atoi( &p[5] );
				}
			}
			// CPT.  For footprint layer info, I don't bother with anything like the SetFileLayerMap business (don't see any point)
			else if( np && key_str == "fp_layer_info" )
			{
				CString file_layer_name = p[0];
				int layer;
				for( layer=0; layer<NUM_FP_LAYERS; layer++ )
				{
					CString layer_string = &fp_layer_str[layer][0];
					if( file_layer_name == layer_string ) break;
				}
				if( layer==NUM_FP_LAYERS )
				{
					CString s ((LPCSTR) IDS_WarningLayerNotSupported), mess;
					mess.Format(s, file_layer_name);
					AfxMessageBox( mess );
				}
				else
				{
					m_fp_rgb[layer][0] = my_atoi( &p[2] );
					m_fp_rgb[layer][1] = my_atoi( &p[3] );
					m_fp_rgb[layer][2] = my_atoi( &p[4] );
					m_fp_vis[layer] = my_atoi( &p[5] );
				}
			}
			else if (np && key_str == "reverse_pgup_pgdn")
				m_bReversePgupPgdn = my_atoi(&p[0]);
			else if (np && key_str == "lefthanded_mode")
				m_bLefthanded = my_atoi(&p[0]);
			else if (np && key_str == "highlight_net")
				m_bHighlightNet = my_atoi(&p[0]);
			else if (np && key_str == "drag_no_sides")
				m_bDragNoSides = my_atoi(&p[0]);
			else if (np && key_str == "error_sound")
				m_bErrorSound = my_atoi(&p[0]);
			else if (np && key_str == "warning_disable")
			{
				int index = my_atoi(&p[0]);
				if (index>=0 && index<NUM_WARNINGS)
					m_bWarningDisable[index] = my_atoi(&p[1]);
			}
		}

		if( m_fp_visible_grid.GetSize() == 0 )
		{
			m_fp_visual_grid_spacing = m_visual_grid_spacing;
			for( int i=0; i<m_visible_grid.GetSize(); i++ )
				m_fp_visible_grid.Add( m_visible_grid[i] );
		}
		if( m_fp_part_grid.GetSize() == 0 )
		{
			m_fp_part_grid_spacing = m_part_grid_spacing;
			for( int i=0; i<m_part_grid.GetSize(); i++ )
				m_fp_part_grid.Add( m_part_grid[i] );
		}
		if( m_fp_snap_angle != 0 && m_fp_snap_angle != 45 && m_fp_snap_angle != 90 )
			m_fp_snap_angle = m_snap_angle;
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		// CPT.  I want a particular order for the grid values in the dropdowns (descending values, mils before mms)
		qsort(m_visible_grid.GetData(), m_visible_grid.GetSize(), sizeof(double), (int (*)(const void*,const void*)) CompareGridVals);
		qsort(m_part_grid.GetData(), m_part_grid.GetSize(), sizeof(double), (int (*)(const void*,const void*)) CompareGridVals);
		qsort(m_routing_grid.GetData(), m_routing_grid.GetSize(), sizeof(double), (int (*)(const void*,const void*)) CompareGridVals);
		qsort(m_fp_visible_grid.GetData(), m_fp_visible_grid.GetSize(), sizeof(double), (int (*)(const void*,const void*)) CompareGridVals);
		qsort(m_fp_part_grid.GetData(), m_fp_part_grid.GetSize(), sizeof(double), (int (*)(const void*,const void*)) CompareGridVals);
		frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
			m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, m_view->m_units );
		m_dlist->SetVisibleGrid( TRUE, m_visual_grid_spacing );
		// CPT2.  The best color choice for LAY_SELECTION & LAY_FP_SELECTION is a bit arbitrary.  Currently the only use for these
		// colors is when user is dragging.  So, how about the ratline color:
		for (int j=0; j<3; j++)
			m_rgb[LAY_SELECTION][j] = m_rgb[LAY_RAT_LINE][j],
			m_fp_rgb[LAY_FP_SELECTION][j] = m_rgb[LAY_RAT_LINE][j];
	}

	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteOptions()\n" + *err_str;
		throw err_str;
	}

}

void AddGridVals(CArray<CString> &arr, CArray<double> &grid, CString label) {
	// CPT: Helper for WriteOptions().  Enters the values in "grid" into "arr", labeling each entry with "label"
	CString str;
	for( int i=0; i<grid.GetSize(); i++ ) {
			if( grid[i] > 0 )
				::MakeCStringFromDimension( &str, grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -grid[i], MM );
			arr.Add( label + str + "\n" );
		}
	arr.Add("\n");
	}

// write project options to file
//
// throws CString * exception on error
//
void CFreePcbDoc::CollectOptionsStrings(CArray<CString> &arr) {
	// CPT: extracted from the original WriteOptions().  
	CString line;
	arr.RemoveAll();
	CFreePcbView * view = (CFreePcbView*)m_view;
	line.Format( "[options]\n\n" );
	arr.Add( line );
	line.Format( "version: %5.3f\n", m_version );
	arr.Add( line );
	line.Format( "file_version: %5.3f\n", m_file_version );
	arr.Add( line );
	line.Format( "project_name: \"%s\"\n", m_name );
	arr.Add( line );
	line.Format( "path_to_folder: \"%s\"\n", m_path_to_folder);
	arr.Add( line );
	line.Format( "library_folder: \"%s\"\n", m_lib_dir );
	arr.Add( line );
	line.Format( "full_library_folder: \"%s\"\n", m_full_lib_dir );
	arr.Add( line );
	// CPT2 the following two options are new:
	line.Format( "sync_file: \"%s\"\n", m_sync_file );
	arr.Add( line );
	line.Format( "sync_file_on: \"%d\"\n", m_bSyncFile );
	arr.Add( line );
	line.Format( "CAM_folder: \"%s\"\n", m_cam_full_path );
	arr.Add( line );
	line.Format( "ses_file_path: \"%s\"\n", m_ses_full_path );
	arr.Add( line );
	line.Format( "netlist_file_path: \"%s\"\n", m_netlist_full_path );
	arr.Add( line );
	line.Format( "SMT_connect_copper: \"%d\"\n", m_bSMT_copper_connect );
	arr.Add( line );
	line.Format( "default_glue_width: \"%d\"\n", m_default_glue_w );
	arr.Add( line );
	line.Format( "dsn_flags: \"%d\"\n", m_dsn_flags );
	arr.Add( line );
	line.Format( "dsn_bounds_poly: \"%d\"\n", m_dsn_bounds_poly );
	arr.Add( line );
	line.Format( "dsn_signals_poly: \"%d\"\n", m_dsn_signals_poly );
	arr.Add( line );
	// CPT autosave_interval, auto_ratline_disable, auto_ratline_disable_min_pins, reverse_pgup_pgdn, etc. are now stored in default.cfg ONLY

	line.Format( "netlist_import_flags: %d\n", m_import_flags );
	arr.Add( line );
	if( m_view->m_units == MIL )
		arr.Add( "units: MIL\n\n" );
	else
		arr.Add( "units: MM\n\n" );
	line.Format( "visible_grid_spacing: %f\n", m_visual_grid_spacing );
	arr.Add( line );
	// CPT: factored out shared code.  Also accounted for hidden grid values:
	AddGridVals(arr, m_visible_grid, "  visible_grid_item: "); 
	AddGridVals(arr, m_visible_grid_hidden, "  visible_grid_hidden: "); 
	line.Format( "placement_grid_spacing: %f\n", m_part_grid_spacing );
	arr.Add( line );
	AddGridVals(arr, m_part_grid, "  placement_grid_item: ");
	AddGridVals(arr, m_part_grid_hidden, "  placement_grid_hidden: ");
	line.Format( "routing_grid_spacing: %f\n", m_routing_grid_spacing );
	arr.Add( line );
	AddGridVals(arr, m_routing_grid, "  routing_grid_item: ");
	AddGridVals(arr, m_routing_grid_hidden, "  routing_grid_hidden: ");
	line.Format( "snap_angle: %d\n", m_snap_angle );
	arr.Add( line );
	arr.Add( "\n" );
	line.Format( "fp_visible_grid_spacing: %f\n", m_fp_visual_grid_spacing );
	arr.Add( line );
	AddGridVals(arr, m_fp_visible_grid, "  fp_visible_grid_item: ");
	AddGridVals(arr, m_fp_visible_grid_hidden, "  fp_visible_grid_hidden: ");
	line.Format( "fp_placement_grid_spacing: %f\n", m_fp_part_grid_spacing );
	arr.Add( line );
	AddGridVals(arr, m_fp_part_grid, "  fp_placement_grid_item: ");
	AddGridVals(arr, m_fp_part_grid_hidden, "  fp_placement_grid_hidden: ");
	line.Format( "fp_snap_angle: %d\n", m_fp_snap_angle );
	arr.Add( line );
	arr.Add( "\n" );
	line.Format( "fill_clearance: %d\n", m_fill_clearance );
	arr.Add( line );
	line.Format( "mask_clearance: %d\n", m_mask_clearance );
	arr.Add( line );
	line.Format( "thermal_width: %d\n", m_thermal_width );
	arr.Add( line );
	line.Format( "min_silkscreen_width: %d\n", m_min_silkscreen_stroke_wid );
	arr.Add( line );
	line.Format( "board_outline_width: %d\n", m_outline_width );
	arr.Add( line );
	line.Format( "hole_clearance: %d\n", m_hole_clearance );
	arr.Add( line );
	line.Format( "pilot_diameter: %d\n", m_pilot_diameter );
	arr.Add( line );
	line.Format( "annular_ring_for_pins: %d\n", m_annular_ring_pins );
	arr.Add( line );
	line.Format( "annular_ring_for_vias: %d\n", m_annular_ring_vias );
	arr.Add( line );
	line.Format( "shrink_paste_mask: %d\n", m_paste_shrink );
	arr.Add( line );
	line.Format( "cam_flags: %d\n", m_cam_flags );
	arr.Add( line );
	line.Format( "cam_layers: %d\n", m_cam_layers );
	arr.Add( line );
	line.Format( "cam_drill_file: %d\n", m_cam_drill_file );
	arr.Add( line );
	line.Format( "cam_units: %d\n", m_cam_units );
	arr.Add( line );
	line.Format( "cam_n_x: %d\n", m_n_x );
	arr.Add( line );
	line.Format( "cam_n_y: %d\n", m_n_y );
	arr.Add( line );
	line.Format( "cam_space_x: %d\n", m_space_x );
	arr.Add( line );
	line.Format( "cam_space_y: %d\n", m_space_y );
	arr.Add( line );
	arr.Add( "\n" );

	line.Format( "report_options: %d\n", m_report_flags );
	arr.Add( line );

	line.Format( "drc_check_unrouted: %d\n", m_dr.bCheckUnrouted );
	arr.Add( line );
	line.Format( "drc_trace_width: %d\n", m_dr.trace_width );
	arr.Add( line );
	line.Format( "drc_pad_pad: %d\n", m_dr.pad_pad );
	arr.Add( line );
	line.Format( "drc_pad_trace: %d\n", m_dr.pad_trace );
	arr.Add( line );
	line.Format( "drc_trace_trace: %d\n", m_dr.trace_trace );
	arr.Add( line );
	line.Format( "drc_hole_copper: %d\n", m_dr.hole_copper );
	arr.Add( line );
	line.Format( "drc_annular_ring_pins: %d\n", m_dr.annular_ring_pins );
	arr.Add( line );
	line.Format( "drc_annular_ring_vias: %d\n", m_dr.annular_ring_vias );
	arr.Add( line );
	line.Format( "drc_board_edge_copper: %d\n", m_dr.board_edge_copper );
	arr.Add( line );
	line.Format( "drc_board_edge_hole: %d\n", m_dr.board_edge_hole );
	arr.Add( line );
	line.Format( "drc_hole_hole: %d\n", m_dr.hole_hole );
	arr.Add( line );
	line.Format( "drc_copper_copper: %d\n", m_dr.copper_copper );
	arr.Add( line );
	arr.Add( "\n" );

	line.Format( "default_trace_width: %d\n", m_trace_w );
	arr.Add( line );
	line.Format( "default_via_pad_width: %d\n", m_via_w );
	arr.Add( line );
	line.Format( "default_via_hole_width: %d\n", m_via_hole_w );
	arr.Add( line );
	line.Format( "n_width_menu: %d\n", m_w.GetSize() );
	arr.Add( line );
	for( int i=0; i<m_w.GetSize(); i++ )
	{
		line.Format( "  width_menu_item: %d %d %d %d\n", i+1, m_w[i], m_v_w[i], m_v_h_w[i]  );
		arr.Add( line );
	}
	arr.Add( "\n" );
	line.Format( "n_copper_layers: %d\n", m_num_copper_layers );
	arr.Add( line );
	for( int i=0; i<(LAY_TOP_COPPER+m_num_copper_layers); i++ )
	{
		line.Format( "  layer_info: \"%s\" %d %d %d %d %d\n",
			&layer_str[i][0], i,
			m_rgb[i][0], m_rgb[i][1], m_rgb[i][2], m_vis[i] );
		arr.Add( line );
	}
	// CPT:  footprint layer info.
	for( int i=0; i<NUM_FP_LAYERS; i++ )
	{
		line.Format( "  fp_layer_info: \"%s\" %d %d %d %d %d\n",
			&fp_layer_str[i][0], i,
			m_fp_rgb[i][0], m_fp_rgb[i][1], m_fp_rgb[i][2], m_fp_vis[i] );
		arr.Add( line );
	}

	arr.Add( "\n" );
}

void CFreePcbDoc::WriteOptions( CStdioFile * file )
{
	try {
		CArray<CString> arr;
		CollectOptionsStrings(arr);
		for (int i=0; i<arr.GetSize(); i++)
			file->WriteString(arr[i]);
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteOptions()\n" + *err_str;									// CPT fixed
		throw err_str;
	}
}

// set defaults for a new project
//
void CFreePcbDoc::InitializeNewProject()
{
	m_dlist->RemoveAll();						// CPT2

	// these are the embedded defaults
	m_name = "";
	m_path_to_folder = "..\\projects\\";
	m_lib_dir = "..\\lib\\" ;
	m_pcb_filename = "";
	m_pcb_full_path = "";
	m_num_copper_layers = 4;
	m_nlist->SetSMTconnect( m_bSMT_copper_connect );
	m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
	m_auto_ratline_disable = FALSE;
	m_auto_ratline_min_pins = 100;
	m_auto_interval = 0;
	m_bReversePgupPgdn = 0;
	m_bLefthanded = 0;
	m_bHighlightNet = 0;
	m_bDragNoSides = 0;
	m_bErrorSound = 1;
	ZeroMemory(&m_bWarningDisable, sizeof(m_bWarningDisable));

	// CPT compacted the code for embedded default layer colors, visibility, etc.
	static unsigned int defaultLayerColors[] = {
		0xff00ff, 0x000000, 0xffffff, 0xffffff, // selection VIOLET, backgrnd BLACK, vis grid WHITE, highlight WHITE
		0xff8040, 0x0000ff, 0xff00ff, 0xffff00, // DRE orange, board outline BLUE, ratlines VIOLET, top silk YELLOW
		0xffc0c0, 0xa0a0a0, 0x5f5f5f, 0x0000ff, // bottom silk PINK, top sm cutout LT GRAY, bottom sm cutout DK GRAY, thru-hole pads BLUE
		0x00ff00, 0xff0000, 0x408040, 0x804040, // top copper GREEN, bottom copper RED, inner 1, inner 2
		0x404080
	};

	static unsigned int defaultFpLayerColors[] = {
		0xff00ff, 0x000000, 0xffffff, 0xffffff, // selection VIOLET, backgrnd BLACK, vis grid WHITE, highlight WHITE
		0xffff00, 0xffc0c0, 0xffffff, 0xff8040, // silk top YELLOW, silk bottom pink, centroid WHITE, dot ORANGE
		0x0000ff, 0x007f00, 0x007f00,			// pad-thru BLUE, Top mask+paste DK GREEN, 
		0x7f0000, 0x7f0000, 0x00ff00, 0x5f5f5f,   // bottom mask+paste DK RED, top copper GREEN, inner copper GRAY, 
		0xff0000								  // bottom copper RED
	};

	for( int i=0; i<MAX_LAYERS; i++ )
	{
		m_vis[i] = 1;
		m_rgb[i][0] = m_rgb[i][1] = m_rgb[i][2] = 127;	// Default grey 
	}
	for (int i=0; i<17; i++)
		m_rgb[i][0] = defaultLayerColors[i]>>16,
		m_rgb[i][1] = defaultLayerColors[i]>>8 & 0xff,
		m_rgb[i][2] = defaultLayerColors[i]&0xff;
	for (int i=0; i<16; i++)
		m_fp_rgb[i][0] = defaultFpLayerColors[i]>>16,
		m_fp_rgb[i][1] = defaultFpLayerColors[i]>>8 & 0xff,
		m_fp_rgb[i][2] = defaultFpLayerColors[i]&0xff;

	// set main screen layer colors & visibility
	for( int i=0; i<m_num_layers; i++ )
	{
		m_vis[i] = 1;
		m_dlist->SetLayerRGB( i, C_RGB(m_rgb[i][0], m_rgb[i][1], m_rgb[i][2]) );
		m_dlist->SetLayerVisible( i, m_vis[i] );
	}
	// now set footprint editor layer colors and visibility
	m_fp_num_layers = NUM_FP_LAYERS;
	for( int i=0; i<m_fp_num_layers; i++ )
	{
		m_fp_vis[i] = 1;
		m_dlist_fp->SetLayerRGB( i, C_RGB(m_fp_rgb[i][0], m_fp_rgb[i][1], m_fp_rgb[i][2]) );
		m_dlist_fp->SetLayerVisible( i, 1 );
	}
	// end CPT

	// CPT:  Also compacted the embedded-defaults code for grid values.  Later we'll see if we can read vals from default.cfg...
	static const double vis_grid_vals[] =
		{ 1000*NM_PER_MIL, 500*NM_PER_MIL, 400*NM_PER_MIL, 250*NM_PER_MIL, 200*NM_PER_MIL, 125*NM_PER_MIL, 100*NM_PER_MIL 
		-100*NM_PER_MM, -50*NM_PER_MM, -40*NM_PER_MM, -25*NM_PER_MM, -20*NM_PER_MM, -10*NM_PER_MM, -5*NM_PER_MM, -2*NM_PER_MM };
	m_visible_grid.RemoveAll();
	m_visible_grid_hidden.RemoveAll();
	for (int i=0; i<15; i++)
		m_visible_grid.Add( vis_grid_vals[i] );
	m_visual_grid_spacing = 200*NM_PER_MIL;
	m_dlist->SetVisibleGrid( TRUE, m_visual_grid_spacing );

	// default placement grid spacing menu values (in NM)
	static const double part_grid_vals[] = 
		{ 1000*NM_PER_MIL, 500*NM_PER_MIL, 400*NM_PER_MIL, 250*NM_PER_MIL, 200*NM_PER_MIL, 100*NM_PER_MIL, 
		  50*NM_PER_MIL, 40*NM_PER_MIL, 25*NM_PER_MIL, 20*NM_PER_MIL, 10*NM_PER_MIL,
		 -10*NM_PER_MM, -5*NM_PER_MM, -4*NM_PER_MM, -2*NM_PER_MM, -1*NM_PER_MM, -.5*NM_PER_MM, -.4*NM_PER_MM, -.2*NM_PER_MM };
	m_part_grid.RemoveAll();
	m_part_grid_hidden.RemoveAll();
	for (int i=0; i<19; i++)
		m_part_grid.Add( part_grid_vals[i] );
	m_part_grid_spacing = 50*NM_PER_MIL;

	// default routing grid spacing menu values (in NM)
	static const double routing_grid_vals[] = 
		{ 100*NM_PER_MIL, 50*NM_PER_MIL, 40*NM_PER_MIL, 25*NM_PER_MIL, 20*NM_PER_MIL, 10*NM_PER_MIL, 
		  5*NM_PER_MIL, 4*NM_PER_MIL, 2.5*NM_PER_MIL, 2*NM_PER_MIL, 1*NM_PER_MIL,
		 -10*NM_PER_MM, -5*NM_PER_MM, -4*NM_PER_MM, -2*NM_PER_MM, -1*NM_PER_MM, -.5*NM_PER_MM, -.4*NM_PER_MM, -.2*NM_PER_MM,
 		 -.1*NM_PER_MM, -.05*NM_PER_MM, -.04*NM_PER_MM, -.02*NM_PER_MM, -.01*NM_PER_MM };
	m_routing_grid.RemoveAll();
	m_routing_grid_hidden.RemoveAll();
	for (int i=0; i<24; i++)
		m_routing_grid.Add( routing_grid_vals[i] );
	m_routing_grid_spacing = 10*NM_PER_MIL;
	
	// footprint editor parameters 
	m_fp_units = MIL;

	// default footprint editor visible grid spacing menu values (in NM)
	static const double fp_vis_grid_vals[] =
		{ 400*NM_PER_MIL, 250*NM_PER_MIL, 200*NM_PER_MIL, 125*NM_PER_MIL, 100*NM_PER_MIL,
		-100*NM_PER_MM, -50*NM_PER_MM, -40*NM_PER_MM, -25*NM_PER_MM, -20*NM_PER_MM };
	m_fp_visible_grid.RemoveAll();
	m_fp_visible_grid_hidden.RemoveAll();
	for (int i=0; i<10; i++)
		m_fp_visible_grid.Add( fp_vis_grid_vals[i] );
	m_fp_visual_grid_spacing = 200*NM_PER_MIL;

	// default footprint editor placement grid spacing menu values (in NM)  (CPT: same as regular placement grid)
	m_fp_part_grid.RemoveAll();
	m_fp_part_grid_hidden.RemoveAll();
	for (int i=0; i<19; i++)
		m_fp_part_grid.Add( part_grid_vals[i] );
	m_fp_part_grid_spacing = 50*NM_PER_MIL;

	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
		m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, MIL );
	// end CPT

	// default PCB parameters
	m_bSMT_copper_connect = FALSE;
	m_default_glue_w = 25*NM_PER_MIL;
	m_trace_w = 10*NM_PER_MIL;
	m_via_w = 28*NM_PER_MIL;
	m_via_hole_w = 14*NM_PER_MIL;
	m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );

	// default cam parameters
	m_dsn_flags = 0;
	m_dsn_bounds_poly = 0;
	m_dsn_signals_poly = 0;
	m_cam_full_path = "";
	m_ses_full_path = "";
	m_fill_clearance = 10*NM_PER_MIL;
	m_mask_clearance = 8*NM_PER_MIL;
	m_thermal_width = 10*NM_PER_MIL;
	m_min_silkscreen_stroke_wid = 5*NM_PER_MIL;
	m_pilot_diameter = 10*NM_PER_MIL;
	m_cam_flags = GERBER_BOARD_OUTLINE | GERBER_NO_CLEARANCE_SMCUTOUTS;
	m_cam_layers = 0xf00fff;	// default layers
	m_cam_units = MIL;
	m_cam_drill_file = 1;
	m_outline_width = 5*NM_PER_MIL;
	m_hole_clearance = 15*NM_PER_MIL;
	m_annular_ring_pins = 7*NM_PER_MIL;
	m_annular_ring_vias = 5*NM_PER_MIL;
	m_paste_shrink = 0;
	m_n_x = 1;
	m_n_y = 1;
	m_space_x = 0;
	m_space_y = 0;

	// default DRC limits
	m_dr.bCheckUnrouted = FALSE;
	m_dr.trace_width = 10*NM_PER_MIL; 
	m_dr.pad_pad = 10*NM_PER_MIL; 
	m_dr.pad_trace = 10*NM_PER_MIL;
	m_dr.trace_trace = 10*NM_PER_MIL; 
	m_dr.hole_copper = 15*NM_PER_MIL; 
	m_dr.annular_ring_pins = 7*NM_PER_MIL;
	m_dr.annular_ring_vias = 5*NM_PER_MIL;
	m_dr.board_edge_copper = 25*NM_PER_MIL;
	m_dr.board_edge_hole = 25*NM_PER_MIL;
	m_dr.hole_hole = 25*NM_PER_MIL;
	m_dr.copper_copper = 10*NM_PER_MIL;

	// Embedded default trace widths (must be in ascending order)
	// CPT streamlined
	int defaultW[] = { 6*NM_PER_MIL, 8*NM_PER_MIL, 10*NM_PER_MIL, 12*NM_PER_MIL, 15*NM_PER_MIL, 20*NM_PER_MIL, 25*NM_PER_MIL };
	int defaultVw[] = { 24*NM_PER_MIL, 24*NM_PER_MIL, 24*NM_PER_MIL, 24*NM_PER_MIL, 30*NM_PER_MIL, 30*NM_PER_MIL, 40*NM_PER_MIL };
	int defaultVhw[] = { 15*NM_PER_MIL, 15*NM_PER_MIL, 15*NM_PER_MIL, 15*NM_PER_MIL, 18*NM_PER_MIL, 18*NM_PER_MIL, 20*NM_PER_MIL };
	m_w.RemoveAll();
	m_v_w.RemoveAll();
	m_v_h_w.RemoveAll();
	for (int i=0; i<7; i++) 
		m_w.Add(defaultW[i]),
		m_v_w.Add(defaultVw[i]),
		m_v_h_w.Add(defaultVhw[i]);
	// end CPT

	// netlist import options
	m_netlist_full_path = "";
	m_import_flags = IMPORT_PARTS | IMPORT_NETS;

	CFreePcbView * view = (CFreePcbView*)m_view;
	view->OnNewProject();								// CPT renamed function

	// now try to find global options file
	// CPT2 because Win7 now write-protects files in \Program Files (curse you, MS), we need to give user an option to change the location of
	// default.cfg.  This is a setting that will have to go in the registry (no better choice, evidently).
	//** AMW3 This doesn't make sense to me. If the application folder is write-protected, then FreePCB can't be installed properly anyway,
	//** because the user has to be able to put files into this folder. It would be better to prompt the user to write-enable the folder
	//** also, if default.cfg is absent, CheckDefaultCfg() apparently creates an empty default.cfg file, which ReadOptions() then tries to read
	//** m_defaultcfg_dir = theApp.GetProfileString(_T("Settings"),_T("DefaultCfgDir"));
	//**	if (m_defaultcfg_dir == "")
		m_defaultcfg_dir = m_app_dir;
	//** CheckDefaultCfg();
	CString fn = m_defaultcfg_dir + "\\default.cfg";
	CStdioFile file;
	if( !file.Open( fn, CFile::modeRead | CFile::typeText ) )
	{
		CString s ((LPCSTR) IDS_UnableToOpenGlobalConfigurationFile);
		AfxMessageBox( s );
	}
	else
	{
		try
		{
			// read global default file options
			ReadOptions( &file );
			// make path to library folder and index libraries
			char full[_MAX_PATH];
			CString fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
			if( fullpath[fullpath.GetLength()-1] == '\\' )	
				fullpath = fullpath.Left(fullpath.GetLength()-1);
			m_full_lib_dir = fullpath;
		}
		catch( CString * err_str )
		{
			*err_str = "CFreePcbDoc::InitializeNewProject()\n" + *err_str;
			throw err_str;
		}
	}
}

// Call this function whenever a user operation is finished,
// or to clear the modified flags.  CPT2 also takes care of closing out the operation's undo record, and as a convenience it also
// calls Redraw().  Finally, it also launches garbage collection every so often.
//
int gGarbageCollectCt = 0;
const int gGarbageCollectFreq = 50;		

void CFreePcbDoc::ProjectModified( BOOL flag, BOOL bCombineWithPreviousUndo )
{
	if (bNoFilesOpened) return;
	CWnd* pMain = AfxGetMainWnd();
	if( flag )
	{
		Redraw();
		FinishUndoRecord( bCombineWithPreviousUndo );						// CPT2.  The new undo system...
		if( m_project_modified )
		{
			// flag already set
			m_project_modified_since_autosave = TRUE;
		}
		else
		{
			// set flags
			CWnd* pMain = AfxGetMainWnd();
			m_project_modified = TRUE;
			m_project_modified_since_autosave = TRUE;
			m_window_title = m_window_title + "*";
			pMain->SetWindowText( m_window_title );
		}
	}
	else
	{
		// clear flags
		m_project_modified = FALSE;
		m_project_modified_since_autosave = FALSE;
		int len = m_window_title.GetLength();
		if( len > 1 && m_window_title[len-1] == '*' )
		{
			m_window_title = m_window_title.Left(len-1);
			pMain->SetWindowText( m_window_title );
		}
		ResetUndoState();
	}
	// enable/disable menu items
	pMain->SetWindowText( m_window_title );
	m_view->SetMainMenu();
	m_view->Invalidate(false);
	// Maybe garbage collect!
	if (++gGarbageCollectCt == gGarbageCollectFreq)
		gGarbageCollectCt = 0,
		GarbageCollect();
}

void CFreePcbDoc::OnViewLayers()
{
	CDlgLayers dlg;
	CFreePcbView * view = (CFreePcbView*)m_view;
	dlg.Initialize( m_num_layers, m_vis, m_fp_vis, m_rgb, m_fp_rgb );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// CPT: For simplicity moved the following out of the DlgLayers::DoDataExchange() code
		for( int i=0; i<NUM_MAIN_LAYERS; i++ )
		{
			m_rgb[i][0] = dlg.m_rgb[i][0];
			m_rgb[i][1] = dlg.m_rgb[i][1];
			m_rgb[i][2] = dlg.m_rgb[i][2];
		}
		// Set footprint layer colors & visibility.  Some of these are borrowed from the main screen layer values; others are based on the last
		// four rows of the layer-dialog
		static int fpLayerMap[] = {	LAY_SELECTION, LAY_BACKGND, LAY_VISIBLE_GRID, LAY_HILITE, LAY_SILK_TOP, -1, -1, LAY_PAD_THRU, 
			-1, -1, -1, -1, -1, LAY_TOP_COPPER, LAY_BOTTOM_COPPER+1, LAY_BOTTOM_COPPER };
		for (int i=0; i<NUM_FP_LAYERS; i++) {
			int mainLayer = fpLayerMap[i];
			if (mainLayer!=-1)
				m_fp_rgb[i][0] = m_rgb[mainLayer][0],
				m_fp_rgb[i][1] = m_rgb[mainLayer][1],
				m_fp_rgb[i][2] = m_rgb[mainLayer][2],
				m_fp_vis[i] = m_vis[mainLayer];
			}
		static int fpLayers[] = { LAY_FP_CENTROID, LAY_FP_DOT, LAY_FP_TOP_MASK, LAY_FP_BOTTOM_MASK };
		for (int i=0; i<4; i++) {
			int layer = fpLayers[i];
			m_fp_rgb[layer][0] = dlg.m_rgb[i+NUM_MAIN_LAYERS][0];
			m_fp_rgb[layer][1] = dlg.m_rgb[i+NUM_MAIN_LAYERS][1];
			m_fp_rgb[layer][2] = dlg.m_rgb[i+NUM_MAIN_LAYERS][2];
			}

		// The footprint top-paste and top-mask colors are always the same on exit.  Likewise for visibility:
		m_fp_rgb[LAY_FP_TOP_PASTE][0] = m_fp_rgb[LAY_FP_TOP_MASK][0];
		m_fp_rgb[LAY_FP_TOP_PASTE][1] = m_fp_rgb[LAY_FP_TOP_MASK][1];
		m_fp_rgb[LAY_FP_TOP_PASTE][2] = m_fp_rgb[LAY_FP_TOP_MASK][2];
		m_fp_rgb[LAY_FP_BOTTOM_PASTE][0] = m_fp_rgb[LAY_FP_BOTTOM_MASK][0];
		m_fp_rgb[LAY_FP_BOTTOM_PASTE][1] = m_fp_rgb[LAY_FP_BOTTOM_MASK][1];
		m_fp_rgb[LAY_FP_BOTTOM_PASTE][2] = m_fp_rgb[LAY_FP_BOTTOM_MASK][2];
		m_fp_vis[LAY_FP_TOP_PASTE] = m_fp_vis[LAY_FP_TOP_MASK];
		m_fp_vis[LAY_FP_BOTTOM_PASTE] = m_fp_vis[LAY_FP_BOTTOM_MASK];

		// CPT2 TODO.  The best color choice for LAY_SELECTION & LAY_FP_SELECTION is a bit of a tricky issue.  Currently the only use for these
		// colors is when user is dragging.  If, say, the background is white and the selection layer is black, then because of the
		// XOR drawing that occurs during dragging, the result is invisible!  To overcome this sort of problem I have (for now) the following kludge:
		for (int j=0; j<3; j++)
			m_rgb[LAY_SELECTION][j] = m_rgb[LAY_RAT_LINE][j],
			m_fp_rgb[LAY_FP_SELECTION][j] = m_rgb[LAY_RAT_LINE][j];

		if (dlg.fColorsDefault) {		
			// User wants to apply settings to future new projects
			CArray<CString> oldLines, newLines;
			CString fn = m_defaultcfg_dir + "\\" + "default.cfg";
			ReadFileLines(fn, oldLines);
			CollectOptionsStrings(newLines);
			ReplaceLines(oldLines, newLines, "layer_info");
			ReplaceLines(oldLines, newLines, "fp_layer_info");
			WriteFileLines(fn, oldLines);
			}

		for( int i=0; i<m_num_layers; i++ )
		{
			m_dlist->SetLayerRGB( i, C_RGB(m_rgb[i][0],m_rgb[i][1],m_rgb[i][2]) );
			m_dlist->SetLayerVisible( i, m_vis[i] );
		}
		view->CancelSelection();
		ProjectModified( TRUE );
		view->Invalidate( FALSE );
	}
}

void CFreePcbDoc::OnProjectPartlist()
{
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	CDlgPartlist dlg;
	if (!view->m_bReplaying)
	{
		dlg.Initialize( m_plist, m_slist, &m_footlibfoldermap, 
			m_view->m_units, m_dlg_log );
		int ret = dlg.DoModal();
		if( ret != IDOK )
			{ view->LogCancel(); return; }
	}
	view->Log(LOG_ON_PROJECT_PARTLIST);
	view->LogPartListInfo( &CDlgPartlist::pli );

	// CPT2 we now call ImportPartListInfo from here (before it was done from CDlgPartlist::DoDataExchange).
	// NB ImportPartListInfo now saves all the undo info we need.
	m_plist->ImportPartListInfo( &CDlgPartlist::pli, 0 );
	view->CancelSelection();
	if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
		m_nlist->OptimizeConnections();
	ProjectModified( TRUE );
}

void CFreePcbDoc::OnFileExport()
{
	CString s ((LPCSTR) IDS_AllFiles);
	CFileDialog dlg( FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_EXPLORER | OFN_OVERWRITEPROMPT, 
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	CString str = dlg.GetPathName();
	CStdioFile file;
	if( !file.Open( str, CFile::modeWrite | CFile::modeCreate ) )
	{
		CString s ((LPCSTR) IDS_UnableToOpenFile2);
		AfxMessageBox( s );
		return;
	}
	CDlgExportOptions dlg2;
	dlg2.Initialize(EXPORT_PARTS | EXPORT_NETS); 
	ret = dlg2.DoModal();
	if (ret != IDOK) 
		return;
	
	// Go for it!
	partlist_info pli;
	netlist_info nli;
	m_plist->ExportPartListInfo( &pli, NULL );
	m_nlist->ExportNetListInfo( &nli );
	if( dlg2.m_format == CMyFileDialog::PADSPCB )
		ExportPADSPCBNetlist( &file, dlg2.m_select, &pli, &nli );
	else
		ASSERT(0);
	file.Close();
}


void CFreePcbDoc::OnFileImport()
{
	CString s ((LPCSTR) IDS_AllFiles), s2 ((LPCSTR) IDS_ImportNetListFile);
	CFileDialog dlg( TRUE , NULL, (LPCTSTR)m_netlist_full_path , OFN_HIDEREADONLY, 
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );
	dlg.m_ofn.lpstrTitle = s2;
	int ret = dlg.DoModal();
	if (ret != IDOK )
		return;
	CString str = dlg.GetPathName(); 
	FileImport(str);
}

void CFreePcbDoc::FileImport(CString path, bool bForceFileSyncCheck)
{
	CStdioFile file;
	if( !file.Open( path, CFile::modeRead ) )
	{
		CString s ((LPCSTR) IDS_UnableToOpenFile2);
		AfxMessageBox( s );
		return;
	}
	// CPT: Do the option dlg even if there are no parts and/or nets in project.  
	// CPT2: But arrange for the appropriate controls to be disabled:
	m_import_flags = 0;
	if (m_plist->parts.IsEmpty())
		m_import_flags |= NO_EXISTING_PARTS;
	if (m_nlist->nets.IsEmpty())
		m_import_flags |= NO_EXISTING_NETS;
	if (bForceFileSyncCheck)
		m_import_flags |= FORCE_FILE_SYNC_CHECK;
	CDlgImportOptions dlg_options;
	dlg_options.Initialize( m_import_flags );
	int ret = dlg_options.DoModal();
	if( ret == IDCANCEL )
		return;
	m_netlist_full_path = path;	// save path for next time

	// Go for it!  First read the netlist file into partlist_info + netlist_info structures.
	m_import_flags = IMPORT_FROM_NETLIST_FILE | dlg_options.m_flags;
	// show log dialog
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	// We will either read the file into a temporary partlist_info + netlist_info, or into the more permanent
	// m_sync_file_pli + m_sync_file_nli, depending on whether user checked the "Sync netlist file" control
	partlist_info pli0, *pli;
	netlist_info nli0, *nli;
	if (m_import_flags & SYNC_NETLIST_FILE)
		m_bSyncFile = true,
		m_sync_file = path,
		pli = &m_sync_file_pli,
		nli = &m_sync_file_nli;
	else
		pli = &pli0,					// Preexisting netlist sync file is unchanged (I guess...)
		nli = &nli0;
	CString line;
	if( dlg_options.m_format == CMyFileDialog::PADSPCB )
	{
		CString s ((LPCSTR) IDS_ReadingNetlistFile);
		line.Format( s, path ); 
		m_dlg_log->AddLine( line );
		int err = ImportPADSPCBNetlist( &file, m_import_flags, pli, nli );
		if( err == NOT_PADSPCB_FILE )
		{
			m_dlg_log->ShowWindow( SW_HIDE );
			CString mess ((LPCSTR) IDS_WarningThisDoesNotAppearToBeALegalPADSPCB);
			int ret = AfxMessageBox( mess, MB_OK );
			return;
		}
	}
	else
		ASSERT(0);
	m_view->CancelSelection();
	if( m_import_flags & SAVE_BEFORE_IMPORT )
		// save project
		OnFileSave();

	if( m_import_flags & IMPORT_PARTS )
	{
		line.LoadStringA(IDS_ImportingPartsIntoProject);
		m_dlg_log->AddLine( line );
		m_plist->ImportPartListInfo( pli, m_import_flags, m_dlg_log );
	}
	if( m_import_flags & IMPORT_NETS )
	{
		line.LoadStringA(IDS_ImportingNetsIntoProject);
		m_dlg_log->AddLine( line );

		// CPT2.  Detach all connects and tees from the existing nets, but save the connects in a temporary array for potential 
		// reattachment later (at which time tees may also get reinstated).  Areas remain in their nets no matter what (differs from the old version).
		CHeap<CConnect> oldConnects;
		CIter<CNet> in (&m_nlist->nets);
		for (CNet *net = in.First(); net; net = in.Next())
			net->MustRedraw(),
			oldConnects.Add( &net->connects ),
			net->connects.RemoveAll(),
			net->tees.RemoveAll();
		m_nlist->ImportNetListInfo( nli, m_import_flags, m_dlg_log, 0, 0, 0 );
		// Now reattach old connects where we can.
		CIter<CConnect> ic (&oldConnects);
		for (CConnect *c = ic.First(); c; c = ic.Next())
		{
			if (c->head->pin && c->tail->pin)
			{
				// Pin-to-pin connect:  if both pins are on the same (new) net, add the connect to that net;  otherwise dump it.
				CNet *net = c->head->pin->net;
				if (net && net == c->tail->pin->net)
					net->AddConnect(c);
			}
			else if (c->head->pin && c->tail->tee)
			{
				// Pin-to-tee connect.  Keep only if the pin is still on c's original net.
				CNet *net = c->head->pin->net;
				if (net == c->m_net)
					net->connects.Add(c),						// No need for net->AddConnect(), since c->m_net isn't changing
					net->tees.Add(c->tail->tee);
			}
			else if (c->tail->pin && c->head->tee)
			{
				// Tee-to-pin connect.  Keep only if the pin is still on c's original net.
				CNet *net = c->tail->pin->net;
				if (net == c->m_net)
					net->connects.Add(c),
					net->tees.Add(c->head->tee);
			}
			else if (c->head->pin)
			{
				// Pin-to-loose-end connect:  keep it if pin is on some (new) net
				CNet *net = c->head->pin->net;
				if (net)
					net->AddConnect(c);
			}
			else if (c->tail->pin)
			{
				// Loose-end-to-pin connect:  keep if pin is on a net
				CNet *net = c->tail->pin->net;
				if (net)
					net->AddConnect(c);
			}
			else
			{
				// Tee-to-tee, tee-to-loose-end, or loose-end-to-loose-end.  Keep it on the net it had before, if that still exists.
				CNet *net = c->m_net;
				if (net->IsOnPcb())
				{
					net->connects.Add(c);
					if (c->head->tee) 
						net->tees.Add(c->head->tee);
					if (c->tail->tee)
						net->tees.Add(c->tail->tee);
				}
			}
		}
	}

	// clean up
	CString str = "\r\n";
	m_nlist->CleanUpAllConnections( &str );
	m_dlg_log->AddLine( str );
	line.LoadStringA(IDS_Done);
	m_dlg_log->AddLine( line );
	// finish up
	m_nlist->OptimizeConnections( FALSE );
	m_view->OnViewAllElements();
	ResetUndoState();	
	ProjectModified( TRUE );
	// make sure log is visible
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
}

int GetSessionLayer( CString * ses_str )
{
	if( *ses_str == "Top" )
		return LAY_TOP_COPPER;
	else if( *ses_str == "Bottom" )
		return LAY_BOTTOM_COPPER;
	else if( ses_str->Left(6) == "Inner_" )
	{
		return( LAY_BOTTOM_COPPER + my_atoi( &(ses_str->Right(1)) ) );
	}
	return -1;
}

// import session file from autorouter
//
void CFreePcbDoc::ImportSessionFile( CString * filepath, CDlgLog * log, BOOL bVerbose )
{
	// process session file
	enum STATE {	// state machine, sub-states indented
	IDLE,
	  PLACEMENT,
	    COMPONENT,
	  ROUTES,
	    PARSER,
	    LIBRARY_OUT,
	      PADSTACK,
	        SHAPE,
	    NETWORK_OUT,
	      NET,
	        VIA,
	        WIRE,
	          PATH
	};
	#define ENDSTATE (field[0] == ")")

	CStdioFile file;
	if( !file.Open( *filepath, CFile::modeRead ) )
	{
		CString mess, s ((LPCSTR) IDS_UnableToOpenSessionFile);
		mess.Format( s, filepath );
		if( log )
			log->AddLine( mess + "\r\n" );
		else
			AfxMessageBox( mess );
		return;
	}
	CArray<CString> field;
	CString instr, units_str, mult_str, footprint_name;
	int mult = 254; // default = 0.01 mil in nm.
	CString net_name, layer_str, width_str, via_name, via_x_str, via_y_str;
	BOOL bNewViaName = FALSE;
	CArray<CNode> nodes;	// array of nodes in net
	CArray<CPath> paths;	// array of paths in net
	CMapStringToPtr via_map;
	STATE state = IDLE;
	while( file.ReadString( instr ) )
	{
		instr.Trim();
		int nf = ParseStringFields( &instr, &field );
		if( nf )
		{
			// IDLE
			if( state == IDLE )
			{
				if( field[0] == "(placement" )
					state = PLACEMENT;
				else if( field[0] == "(routes" )
					state = ROUTES;
			}
			// IDLE -> PLACEMENT
			else if( state == PLACEMENT )
			{
				if( ENDSTATE )
					state = IDLE;
				else if( field[0] == "(component" )
				{
					state = COMPONENT;
					footprint_name = field[1];
				}
				else if( field[0] == "(resolution" )
				{
					units_str = field[1];
					mult_str = field[2];
					if( units_str == "mil" )
					{
						mult = my_atoi( &mult_str );
						mult = 25400/mult;
					}
					else
						ASSERT(0);
				}
			}
			// IDLE -> PLACEMENT -> COMPONENT
			else if( state == COMPONENT )
			{
				if( ENDSTATE )
					state = PLACEMENT;
				else if( field[0] == "(place" )
				{
				}
			}
			// IDLE -> ROUTES
			else if( state == ROUTES )
			{
				if( ENDSTATE )
					state = IDLE;
				else if( field[0] == "(resolution" )
				{
					units_str = field[1];
					mult_str = field[2];
					if( units_str == "mil" )
					{
						mult = my_atoi( &mult_str );
						mult = 25400/mult;
					}
					else
						ASSERT(0);
				}
				else if( field[0] == "(parser" )
					state = PARSER;
				else if( field[0] == "(library_out" )
					state = LIBRARY_OUT;
				else if( field[0] == "(network_out" )
					state = NETWORK_OUT;
			}
			// IDLE -> ROUTES -> PARSER
			else if( state == PARSER )
			{
				if( ENDSTATE )
					state = ROUTES;
			}
			// IDLE -> ROUTES -> LIBRARY_OUT
			else if( state == LIBRARY_OUT )
			{
				if( ENDSTATE )
					state = ROUTES;
				else if( field[0] == "(padstack" )
				{
					state = PADSTACK;
					via_name = field[1];
					bNewViaName = TRUE;
				}
			}
			// IDLE -> ROUTES -> LIBRARY_OUT -> PADSTACK
			else if( state == PADSTACK )
			{
				if( ENDSTATE )
					state = LIBRARY_OUT;
				else if( field[0] == "(shape" )
					state = SHAPE;
			}
			// IDLE -> ROUTES -> LIBRARY_OUT -> PADSTACK -> SHAPE
			else if( state == SHAPE )
			{
				if( ENDSTATE )
					state = PADSTACK;
				else if( field[0] == "(circle" && bNewViaName )
				{
					// add via definition to via_map
					CString via_w_str = field[2];
					int via_w = mult * my_atoi( &via_w_str );
					via_map.SetAt( via_name, (void*)via_w );
					bNewViaName = FALSE;
				}
			}
			// IDLE -> ROUTES -> NETWORK_OUT
			else if( state == NETWORK_OUT )
			{
				if( ENDSTATE )
					state = ROUTES;
				else if( field[0] == "(net" )
				{
					state = NET;
					net_name = field[1];
					nodes.SetSize(0);
					paths.SetSize(0);
				}
			}
			// IDLE -> ROUTES -> NETWORK_OUT -> NET
			else if( state == NET )
			{
				if( ENDSTATE )
				{
					// end of data for this net, route project
					CNet *net = m_nlist->GetNetPtrByName( &net_name );
					net->ImportRouting( &nodes, &paths, mult, log, bVerbose );
					state = NETWORK_OUT;
				}
				else if( field[0] == "(via" )
				{
					// data for a via
					state = VIA;
					via_name = field[1];
					void * ptr;
					BOOL bOK = via_map.Lookup( via_name, ptr );
					if( bOK )
					{
						via_x_str = field[2];
						via_y_str = field[3];
						int inode = nodes.GetSize();
						nodes.SetSize(inode+1);
						nodes[inode].type = NVIA;
						nodes[inode].x = mult * my_atoi( &via_x_str );
						nodes[inode].y = mult * my_atoi( &via_y_str );
						nodes[inode].layer = LAY_PAD_THRU;
						nodes[inode].via_w = (int)ptr;
					}
					else
						ASSERT(0);
				}
				else if( field[0] == "(wire" )
					state = WIRE;
			}
			// IDLE -> ROUTES -> NETWORK_OUT -> NET -> VIA
			else if( state == VIA )
			{
				if( ENDSTATE )
					state = NET;
			}
			// IDLE -> ROUTES -> NETWORK_OUT -> NET -> WIRE
			else if( state == WIRE )
			{
				if( ENDSTATE )
					state = NET;
				else if( field[0] == "(path" )
				{
					// path data
					state = PATH;
					layer_str = field[1];
					width_str = field[2];
					int ipath = paths.GetSize();
					paths.SetSize( ipath+1 );
					paths[ipath].layer = GetSessionLayer( &layer_str );
					paths[ipath].width = mult * my_atoi( &width_str );
					paths[ipath].bUsed = false;
				}
			}
			// IDLE -> ROUTES -> NETWORK_OUT -> NET -> WIRE -> PATH
			else if( state == PATH )
			{
				if( ENDSTATE )
					state = WIRE;
				else
				{
					// path point data
					CString x_str = field[0];
					CString y_str = field[1];
					int ipath = paths.GetSize()-1;
					int ipt = paths[ipath].pt.GetSize();
					paths[ipath].pt.SetSize( ipt+1 );
					paths[ipath].pt[ipt].x = mult * my_atoi( &x_str );;
					paths[ipath].pt[ipt].y = mult * my_atoi( &y_str );;
					paths[ipath].pt[ipt].inode = -1;
				}
			}
		}
	}
	file.Close();
}

// import netlist 
// enter with file already open
// CPT2 TODO IS THIS DEFUNCT?
//
/*
int CFreePcbDoc::ImportNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pli, netlist_info * nli )
{
	CString instr;
	int err_flags = 0;
	int line = 0;
	BOOL not_eof;

	// find next section
	not_eof = file->ReadString( instr );
	line++;
	instr.Trim();
	while( 1 )
	{
		if( instr == "[parts]" && pli && (flags & IMPORT_PARTS) )
		{
			// read parts
			int ipart = 0;
			BOOL parts_section = FALSE;
			while( 1 )
			{
				not_eof = file->ReadString( instr );
				if( !not_eof )
				{
					// end of file
					file->Close();
					return err_flags;
				}
				line++;
				instr.Trim();
				if( instr[0] == '[' )
				{
					// next section
					break;
				}
				else if( instr.GetLength() && instr[0] != '/' )
				{
					// get ref prefix, ref number and shape
					pli->SetSize( ipart+1 );
					CString ref_str( mystrtok( instr, " \t" ) );
					CString shape_str( mystrtok( NULL, "\n\r" ) );
					shape_str.Trim();
					// find footprint, get from library if necessary.  CPT2 TODO here are the null shapes.  Disable?
					CShape * s = GetFootprintPtr( &shape_str );
					// add part to partlist_info
					(*pli)[ipart].part = NULL;
					(*pli)[ipart].ref_des = ref_str;
					if( s )
					{
						(*pli)[ipart].ref_size = s->m_ref->m_font_size;
						(*pli)[ipart].ref_width = s->m_ref->m_stroke_width;
					}
					else
					{
						(*pli)[ipart].ref_size = 0;
						(*pli)[ipart].ref_width = 0;
					}
					(*pli)[ipart].package = shape_str;
					(*pli)[ipart].shape = s;
					(*pli)[ipart].x = 0;
					(*pli)[ipart].y = 0;
					(*pli)[ipart].angle = 0;
					(*pli)[ipart].side = 0;
					if( !s )
						err_flags |= FOOTPRINTS_NOT_FOUND;
					ipart++;
				}
			}
		}
		else if( instr == "[nets]" && nli && (flags & IMPORT_NETS) )
		{
			// read nets
			int num_pins = 0;
			while( 1 )
			{
				not_eof = file->ReadString( instr );
				if( !not_eof )
				{
					// end of file
					file->Close();
					return err_flags;
				}
				line++;
				instr.Trim();
				if( instr[0] == '[' )
				{
					// next section
					break;
				}
				else if( instr.GetLength() && instr[0] != '/' )
				{
					int delim_pos;
					if( (delim_pos = instr.Find( ":", 0 )) != -1 )
					{
						// new net, get net name
						int inet = nli->GetSize();
						nli->SetSize( inet+1 );
						CString net_name( mystrtok( instr, ":" ) );
						net_name.Trim();
						if( net_name.GetLength() )
						{
							// add new net
							(*nli)[inet].name = net_name;
							(*nli)[inet].net = NULL;
							(*nli)[inet].modified = TRUE;
							(*nli)[inet].deleted = FALSE;
							(*nli)[inet].visible = TRUE;
							(*nli)[inet].w = 0;
							(*nli)[inet].v_w = 0;
							(*nli)[inet].v_h_w = 0;
							instr = instr.Right( instr.GetLength()-delim_pos-1 );
							num_pins = 0;
						}
						// add pins to net
						char * pin = mystrtok( instr, " \t\n\r" );
						while( pin )
						{
							CString pin_cstr( pin );
							if( pin_cstr.GetLength() > 3 )
							{
								int dot = pin_cstr.Find( ".", 0 );
								if( dot )
								{
									CString ref_des = pin_cstr.Left( dot );
									CString pin_num_cstr = pin_cstr.Right( pin_cstr.GetLength()-dot-1 );
									(*nli)[inet].ref_des.Add( ref_des );
									(*nli)[inet].pin_name.Add( pin_num_cstr );
#if 0	// TODO: check for illegal pin names
									}
									else
									{
										// illegal pin number for part
										CString mess;
										mess.Format( "Error in line %d of netlist file\nIllegal pin number \"%s\"", 
											line, pin_cstr );
										AfxMessageBox( mess );
										break;
									}
#endif
								}
								else
								{
									// illegal string
									break;
								}
							}
							else if( pin_cstr != "" )
							{
								// illegal pin identifier
								CString mess, s ((LPCSTR) IDS_ErrorInLineOfNetlistFile);
								mess.Format( s,	line, pin_cstr );
								AfxMessageBox( mess );
							}
							pin = mystrtok( NULL, " \t\n\r" );
						} // end while( pin )
					}
				}
			}
		}
		else if( instr == "[end]" )
		{
			// normal return
			file->Close();
			return err_flags;
		}
		else
		{
			not_eof = file->ReadString( instr );
			line++;
			instr.Trim();
			if( !not_eof)
			{
				// end of file
				file->Close();
				return err_flags;
			}
		}
	}
}
*/

// export netlist in PADS-PCB format
// enter with file already open
// flags:
//	EXPORT_PARTS = include parts in file
//	EXPORT_NETS = include nets in file
//	EXPORT_VALUES = use "value@footprint" format for parts
// CPT:  added sorting so that results are more readable.

int CFreePcbDoc::ExportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pli, netlist_info * nli )
{
	CString str, str2;
	file->WriteString( "*PADS-PCB*\n" );
	if( flags & EXPORT_PARTS )
	{
		file->WriteString( "*PART*\n" );
		CArray<CString> parts;							// Will accumulate part strings in this array, and sort afterwards.
		for( int i=0; i<pli->GetSize(); i++ )
		{
			part_info * pi = &(*pli)[i];
			str2 = "";
			if( flags & EXPORT_VALUES && pi->value != "" )
				str2 = pi->value + "@";
			if( pi->shape )
				str2 += pi->shape->m_name;
			else
				str2 += pi->package;
			str.Format( "%s %s\n", pi->ref_des, str2 );
			parts.Add( str );
		}
		qsort(parts.GetData(), parts.GetSize(), sizeof(CString), (int (*)(const void*,const void*)) CompareNumeric);			
		for (int i=0; i<parts.GetSize(); i++) 
			file->WriteString(parts[i]);
	}

	if( flags & EXPORT_NETS )
	{
		if( flags & IMPORT_PARTS )
			file->WriteString( "\n" );
		file->WriteString( "*NET*\n" );

		CArray<CString> nets;							// Will accumulate net strings in this array, and sort afterwards.
		for( int i=0; i<nli->GetSize(); i++ )
		{
			net_info * ni = &(*nli)[i];
			str.Format( "*SIGNAL* %s", ni->name );
			CArray<CString> pins;
			int np = ni->pin_name.GetSize();
			for( int ip=0; ip<np; ip++ )
			{
				CString pin_str;
				pin_str.Format( "%s.%s ", ni->ref_des[ip], ni->pin_name[ip] );
				pins.Add(pin_str);
			}
			qsort(pins.GetData(), pins.GetSize(), sizeof(CString), (int (*)(const void*,const void*)) CompareNumeric);			
			for (int i=0; i<np; i++) 
			{
				if (!(i%8)) str += "\n  ";
				str += pins[i];
			}
			nets.Add(str);
		}

		qsort(nets.GetData(), nets.GetSize(), sizeof(CString), (int (*)(const void*,const void*)) CompareNumeric);			
		for (int i=0; i<nets.GetSize(); i++) 
			file->WriteString(nets[i]),
			file->WriteString("\n");
	}
	file->WriteString( "*END*\n" );
	return 0;
}
// (end CPT)

// import netlist in PADS-PCB format
// enter with file already open
//
int CFreePcbDoc::ImportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pli, netlist_info * nli )
{
	CString instr, net_name, mess;
	CMapStringToPtr part_map, net_map, pin_map;
	void * ptr;
	int npins, inet = 0, ipart = 0;
	int err_flags = 0, line = 0;
	BOOL not_eof;
	// state machine
	enum { IDLE, PARTS, NETS, SIGNAL };
	int state = IDLE;
	BOOL bLegal = FALSE;

	// CPT2: Empty out preexisting contents of pli and nli
	pli->RemoveAll();
	nli->RemoveAll();

	while( 1 )
	{
		not_eof = file->ReadString( instr );
		line++;
		instr.Trim();
		if( line > 2 && !bLegal )
		{
			file->Close();
			return NOT_PADSPCB_FILE;
		}
		if( instr.Left(5) == "*END*" || !not_eof )
		{
			// normal return
			file->Close();
			return err_flags;
		}
		else if( instr.Left(10) == "*PADS-PCB*" || instr.Left(10) == "*PADS2000*" )
			bLegal = TRUE;
		else if( instr.Left(6) == "*PART*" )
			state = PARTS;
		else if( instr.Left(5) == "*NET*" )
			state = NETS;
		else if( state == PARTS && pli && (flags & IMPORT_PARTS) )
		{
			// read parts
			if( instr.GetLength() && instr[0] != '/' )
			{
				// get ref_des and footprint
				CString ref_str( mystrtok( instr, " \t" ) );
				if( ref_str.GetLength() > MAX_REF_DES_SIZE )
				{
					CString mess, s ((LPCSTR) IDS_LineReferenceDesignatorTooLong);
					mess.Format( s, line, ref_str );
					m_dlg_log->AddLine( mess );
					ref_str = ref_str.Left(MAX_REF_DES_SIZE);
				}
				// check for legal ref_designator
				if( ref_str.FindOneOf( ". " ) != -1 )
				{
					CString mess, s ((LPCSTR) IDS_LinePartIllegalReferenceDesignator);
					mess.Format( s,	line, ref_str );
					m_dlg_log->AddLine( mess );
					continue;
				}
				// check for duplicate part
				if( part_map.Lookup( ref_str, ptr ) )
				{
					CString mess, s ((LPCSTR) IDS_LinePartIsDuplicateIgnored);
					mess.Format( s,	line, ref_str );
					m_dlg_log->AddLine( mess );
					continue;
				}
				// new part
				pli->SetSize( ipart+1 );
				part_info *pi = &(*pli)[ipart];
				CString shape_str( mystrtok( NULL, "\n\r" ) );
				shape_str.Trim();
				// check for "ssss@ffff" format
				int pos = shape_str.Find( "@" );
				if( pos != -1 )
				{
					CString value_str;
					SplitString( &shape_str, &value_str, &shape_str, '@' );
					pi->value = value_str;
				}
				if( shape_str.GetLength() > CShape::MAX_NAME_SIZE )
				{
					CString mess, s ((LPCSTR) IDS_LinePackageNameTooLong);
					mess.Format( s,	line, shape_str );
					m_dlg_log->AddLine( mess );
					shape_str = shape_str.Left(CShape::MAX_NAME_SIZE);
				}
				// find footprint, get from library if necessary
				CShape * s = NULL;
				if (!(flags & DONT_LOAD_SHAPES))
				{
					s = GetFootprintPtr( &shape_str );
					if( !s )
					{
						CString mess, s ((LPCSTR) IDS_LinePartFootprintNotFound);
						mess.Format( s,	line, ref_str, shape_str );
						m_dlg_log->AddLine( mess );
					}
				}
				// add part to partlist_info
				pi->part = NULL;
				pi->ref_des = ref_str;
				part_map.SetAt( ref_str, NULL );
				if( s )
				{
					pi->ref_size = s->m_ref->m_font_size;
					pi->ref_width = s->m_ref->m_stroke_width;
				}
				else
					pi->ref_size = pi->ref_width = 0;
				pi->package = shape_str;
				pi->bOffBoard = TRUE;
				pi->shape = s;
				pi->shape_name = shape_str;		// CPT2 new
				pi->angle = 0;
				pi->side = 0;
				pi->x = pi->y = 0;
				pi->ref_vis = true;		// CPT
				ipart++;
			}
		}
		else if( instr.Left(8) == "*SIGNAL*" && nli && (flags & IMPORT_NETS) )
		{
			state = NETS;
			net_name = instr.Right(instr.GetLength()-8);
			net_name.Trim();
			int pos = net_name.Find( " " );
			if( pos != -1 )
			{
				net_name = net_name.Left( pos );
			}
			if( net_name.GetLength() )
			{
				if( net_name.GetLength() > MAX_NET_NAME_SIZE )
				{
					CString mess, s ((LPCSTR) IDS_LineNetNameTooLong);
					mess.Format( s,	line, net_name, net_name.Left(MAX_NET_NAME_SIZE) );
					m_dlg_log->AddLine( mess );
					net_name = net_name.Left(MAX_NET_NAME_SIZE);
				}
				if( net_name.FindOneOf( " \"" ) != -1 )
				{
					CString mess, s ((LPCSTR) IDS_LineNetNameIllegal);
					mess.Format( s, line, net_name );
					m_dlg_log->AddLine( mess );
				}
				else
				{
					if( net_map.Lookup( net_name, ptr ) )
					{
						CString mess, s ((LPCSTR) IDS_LineNetNameIsDuplicate);
						mess.Format( s,	line, net_name );
						m_dlg_log->AddLine( mess );
					}
					else
					{
						// add new net
						net_map.SetAt( net_name, NULL );
						inet = nli->GetSize();
						nli->SetSize( inet+1 );
						net_info *ni = &(*nli)[inet];
						ni->name = net_name;
						ni->net = NULL;
						ni->apply_trace_width = FALSE;
						ni->apply_via_width = FALSE;
						ni->modified = TRUE;
						ni->deleted = FALSE;
						ni->visible = TRUE;
						// mark widths as undefined
						ni->w = -1;
						ni->v_w = -1;
						ni->v_h_w = -1;
						npins = 0;
						state = SIGNAL;
					}
				}
			}
		}
		else if( state == SIGNAL && nli && (flags & IMPORT_NETS) )
		{
			// add pins to net
			char * pin = mystrtok( instr, " \t\n\r" );
			while( pin )
			{
				CString pin_cstr( pin );
				if( pin_cstr.GetLength() > 3 )
				{
					int dot = pin_cstr.Find( ".", 0 );
					if( dot )
					{
						if( pin_map.Lookup( pin_cstr, ptr ) )
						{
							CString mess, s ((LPCSTR) IDS_LineNetPinIsDuplicate);
							mess.Format( s,	line, net_name, pin_cstr );
							m_dlg_log->AddLine( mess );
						}
						else
						{
							pin_map.SetAt( pin_cstr, NULL );
							CString ref_des = pin_cstr.Left( dot );
							CString pin_num_cstr = pin_cstr.Right( pin_cstr.GetLength()-dot-1 );
							(*nli)[inet].ref_des.Add( ref_des );
							if( pin_num_cstr.GetLength() > CShape::MAX_PIN_NAME_SIZE )
							{
								CString mess, s ((LPCSTR) IDS_LinePinNameTooLong);
								mess.Format( s,	line, pin_num_cstr );
								m_dlg_log->AddLine( mess );
								pin_num_cstr = pin_num_cstr.Left(CShape::MAX_PIN_NAME_SIZE);
							}
							(*nli)[inet].pin_name.Add( pin_num_cstr );
						}
					}
					else
					{
						// illegal pin identifier
						CString mess, s ((LPCSTR) IDS_LinePinIdentifierIllegal);
						mess.Format( s,	line, pin_cstr );
						m_dlg_log->AddLine( mess );
					}
				}
				else
				{
					// illegal pin identifier
					CString mess, s ((LPCSTR) IDS_LinePinIdentifierIllegal);
					mess.Format( s, line, pin_cstr );
					m_dlg_log->AddLine( mess );
				}
				pin = mystrtok( NULL, " \t\n\r" );
			} // end while( pin )
		}
	} // end while
}

void CFreePcbDoc::OnAppExit()
{
	if( FileClose() != IDCANCEL )
	{
//		m_view->SetHandleCmdMsgFlag( FALSE );
		GarbageCollect();	//** AMW3
		AfxGetMainWnd()->SendMessage( WM_CLOSE, 0, 0 );
	}
}

void CFreePcbDoc::OnFileConvert()
{
	CivexDlg dlg;
	dlg.DoModal();
}


// call dialog to create Gerber and drill files
void CFreePcbDoc::OnFileGenerateCadFiles()
{
	if( boards.GetSize() == 0 )
	{
		CString s ((LPCSTR) IDS_ABoardOutlineMustBePresentForCAM);
		AfxMessageBox( s );
		return;
	}
	CDlgCAD dlg;
	if( m_cam_full_path == "" )
		m_cam_full_path = m_path_to_folder + "\\CAM";
	dlg.Initialize( m_version,
		&m_cam_full_path, 
		&m_path_to_folder,
		&m_app_dir,
		m_num_copper_layers, 
		m_cam_units,
		m_bSMT_copper_connect,
		m_fill_clearance, 
		m_mask_clearance,
		m_thermal_width,
		m_pilot_diameter,
		m_min_silkscreen_stroke_wid,
		m_outline_width,
		m_hole_clearance,
		m_annular_ring_pins,
		m_annular_ring_vias,
		m_paste_shrink,
		m_n_x, m_n_y, m_space_x, m_space_y,
		m_cam_flags,
		m_cam_layers,
		m_cam_drill_file,
		&boards,
		&smcutouts,
		m_plist, 
		m_nlist, 
		m_tlist, 
		m_dlist,
		m_dlg_log );
	m_nlist->OptimizeConnections( FALSE );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// update parameters
		if( m_cam_full_path != dlg.m_folder
			|| m_cam_units != dlg.m_units
			|| m_fill_clearance != dlg.m_fill_clearance
			|| m_mask_clearance != dlg.m_mask_clearance
			|| m_thermal_width != dlg.m_thermal_width
			|| m_min_silkscreen_stroke_wid != dlg.m_min_silkscreen_width
			|| m_pilot_diameter != dlg.m_pilot_diameter
			|| m_outline_width != dlg.m_outline_width
			|| m_hole_clearance != dlg.m_hole_clearance
			|| m_annular_ring_pins != dlg.m_annular_ring_pins
			|| m_annular_ring_vias != dlg.m_annular_ring_vias
			|| m_cam_flags != dlg.m_flags
			|| m_cam_layers != dlg.m_layers
			|| m_cam_drill_file != dlg.m_drill_file )
		{
			ProjectModified( TRUE );
		}
		m_cam_full_path = dlg.m_folder;
		m_cam_units = dlg.m_units;
		m_fill_clearance = dlg.m_fill_clearance;
		m_mask_clearance = dlg.m_mask_clearance;
		m_thermal_width = dlg.m_thermal_width;
		m_min_silkscreen_stroke_wid = dlg.m_min_silkscreen_width;
		m_pilot_diameter = dlg.m_pilot_diameter;
		m_outline_width = dlg.m_outline_width;
		m_hole_clearance = dlg.m_hole_clearance;
		m_annular_ring_pins = dlg.m_annular_ring_pins;
		m_annular_ring_vias = dlg.m_annular_ring_vias;
		m_paste_shrink = dlg.m_paste_shrink;
		m_cam_flags = dlg.m_flags;
		m_cam_layers = dlg.m_layers;
		m_cam_drill_file = dlg.m_drill_file;
		m_n_x = dlg.m_n_x;
		m_n_y = dlg.m_n_y;
		m_space_x = dlg.m_space_x;
		m_space_y = dlg.m_space_y;
	}
}

void CFreePcbDoc::OnToolsFootprintwizard()
{
	CDlgWizQuad dlg;
	dlg.Initialize( m_slist, &m_footlibfoldermap, TRUE, m_dlg_log );
	dlg.DoModal();
}

void CFreePcbDoc::MakeLibraryMaps( CString * fullpath )
{
	m_footlibfoldermap.SetDefaultFolder( fullpath );
	m_footlibfoldermap.AddFolder( fullpath, NULL );
}

void CFreePcbDoc::OnProjectOptions()
{
	CDlgProjectOptions dlg;
	if( m_name == "" )
	{
		m_name = m_pcb_filename;
		if( m_name.Right(4) == ".fpc" )
			m_name = m_name.Left( m_name.GetLength()-4 );
	}
	// CPT: args have changed:
	dlg.Init( FALSE, &m_name, &m_path_to_folder, &m_lib_dir, m_bSyncFile, &m_sync_file,
		m_num_copper_layers, m_bSMT_copper_connect, m_default_glue_w,
		m_trace_w, m_via_w, m_via_hole_w,
		&m_w, &m_v_w, &m_v_h_w );
	// end CPT

	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	// set options from dialog
	BOOL bResetAreaConnections = m_bSMT_copper_connect != dlg.m_bSMT_connect_copper;
	m_bSMT_copper_connect = dlg.m_bSMT_connect_copper;
	m_nlist->SetSMTconnect( m_bSMT_copper_connect );
	m_default_glue_w = dlg.GetGlueWidth();
	// deal with decreased number of layers
	if( m_num_copper_layers > dlg.GetNumCopperLayers() )
	{
		// decreasing number of layers, offer to reassign them
		CDlgReassignLayers rel_dlg;
		rel_dlg.Initialize( m_num_copper_layers, dlg.GetNumCopperLayers() );
		int ret = rel_dlg.DoModal();
		if( ret == IDOK )
		{
			// reassign copper layers
			m_nlist->ReassignCopperLayers( dlg.GetNumCopperLayers(), rel_dlg.new_layer );
			m_tlist->ReassignCopperLayers( dlg.GetNumCopperLayers(), rel_dlg.new_layer );
			m_num_copper_layers = dlg.GetNumCopperLayers();
			m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
		}
		// clear clipboard
		clip_nlist->nets.RemoveAll();
		clip_plist->parts.RemoveAll();
		clip_tlist->texts.RemoveAll();
		clip_smcutouts.RemoveAll();
		clip_boards.RemoveAll();
	}
	else if( m_num_copper_layers < dlg.GetNumCopperLayers() )
	{
		// increasing number of layers, don't reassign
		for( int il=m_num_copper_layers; il<dlg.GetNumCopperLayers(); il++ )
			m_vis[LAY_TOP_COPPER+il] = 1;
		m_num_copper_layers = dlg.GetNumCopperLayers();
		m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
	}

	m_name = dlg.GetName();
	m_lib_dir = dlg.GetLibFolder();							// CPT added (not sure why it wasn't there before...)
	if( m_full_lib_dir != dlg.GetLibFolder() )
	{
		m_full_lib_dir = dlg.GetLibFolder();
		m_footlibfoldermap.SetDefaultFolder( &m_full_lib_dir );		
		m_footlibfoldermap.SetLastFolder( &m_full_lib_dir );		
	}
	m_trace_w = dlg.GetTraceWidth();
	m_via_w = dlg.GetViaWidth();
	m_via_hole_w = dlg.GetViaHoleWidth();
	m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
	// CPT2 new
	m_bSyncFile = dlg.m_bSyncFile;
	m_sync_file = dlg.m_sync_file;

	// CPT: option to save dlg results to default.cfg:
	if (dlg.m_default) 
	{
		CArray<CString> oldLines, newLines;
		CString fn = m_defaultcfg_dir + "\\" + "default.cfg";
		ReadFileLines(fn, oldLines);
		CollectOptionsStrings(newLines);
		ReplaceLines(oldLines, newLines, "path_to_folder");
		ReplaceLines(oldLines, newLines, "library_folder");
		ReplaceLines(oldLines, newLines, "n_copper_layers");
		ReplaceLines(oldLines, newLines, "SMT_connect_copper");
		ReplaceLines(oldLines, newLines, "default_glue_width");
		ReplaceLines(oldLines, newLines, "default_trace_width");
		ReplaceLines(oldLines, newLines, "default_via_pad_width");
		ReplaceLines(oldLines, newLines, "default_via_hole_width");
		ReplaceLines(oldLines, newLines, "n_width_menu");
		ReplaceLines(oldLines, newLines, "width_menu_item");
		WriteFileLines(fn, oldLines);
	}
	// end CPT

	if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
		m_nlist->OptimizeConnections();
	m_project_open = TRUE;

	// force redraw of function key text
	m_view->m_cursor_mode = 999;
	m_view->CancelSelection();
	//** AMW3 force redraw of all parts if m_bSMT_copper_connect changed
	if( bResetAreaConnections )
		m_plist->SetThermals();
	//** end AMW3
	ProjectModified( TRUE );
	ResetUndoState();

	// CPT2 new:
	if (m_bSyncFile)
	{
		CString s ((LPCSTR) IDS_DoYouWishToImportFromTheNetlistFileNow);
		int ret = AfxMessageBox( s, MB_YESNO );
		if (ret==IDYES)
			FileImport(m_sync_file, true);
		else
			// Still need to load in the netlist file to m_sync_file_pli and m_sync_file_nli.
			LoadSyncFile();
	}
}

// come here from MainFrm on timer event
//
void CFreePcbDoc::OnTimer()
{
	if( m_project_modified_since_autosave )
	{
		m_auto_elapsed += TIMER_PERIOD;
		if( m_view && m_auto_interval && m_auto_elapsed > m_auto_interval )
		{
			if( !m_view->CurDragging() )
				AutoSave();
		}
	}
}


void CFreePcbDoc::OnToolsCheckPartsAndNets()
{
	// open log
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	view->Log(LOG_ON_TOOLS_CHECK_PARTS_AND_NETS);
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	CString str;
	int nerrors = m_plist->CheckPartlist( &str );
	str += "\r\n";
	nerrors += m_nlist->CheckNetlist( &str );
	m_dlg_log->AddLine( str );
}


void CFreePcbDoc::SetFileLayerMap( int file_layer, int layer )
{
	m_layer_by_file_layer[file_layer] = layer;
}


void CFreePcbDoc::OnToolsCheckConnectivity()
{
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	view->Log(LOG_ON_TOOLS_CHECK_CONNECTIVITY);
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	CString str;
	int nerrors = m_nlist->CheckConnectivity( &str );
	m_dlg_log->AddLine( str );
	if( !nerrors )
	{
		str.LoadStringA(IDS_NoUnroutedConnections);
		m_dlg_log->AddLine( str );
	}
	else
		ResetUndoState();					// CPT2, just to be safe in the case that fixes were applied
}

void CFreePcbDoc::OnViewLog()
{
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
}

void CFreePcbDoc::OnToolsCheckCopperAreas()
{
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	view->Log(LOG_ON_TOOLS_CHECK_COPPER_AREAS);
	CString str;
 
	m_dlg_log->ShowWindow( SW_SHOW );   
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	m_view->CancelSelection();
	CIter<CNet> in (&m_nlist->nets);
	for (CNet *net = in.First(); net; net = in.Next())
	{
		if( net->NumAreas() == 0 )
			continue;
		CString s ((LPCSTR) IDS_NetAreas);
		str.Format( s, net->name, net->NumAreas() ); 
		m_dlg_log->AddLine( str );
		net->SaveUndoInfo( CNet::SAVE_AREAS );

		CIter<CArea> ia (&net->areas);
		for (CArea *a = ia.First(); a; a = ia.Next())
		{
			// CPT2 in addition to giving messages, I'll have these errant areas removed:
			int nc = a->NumCorners();
			if( nc < 3 )
			{
				s.LoadStringA(IDS_AreaHasOnlyCorners);
				str.Format( s, a->UID(), nc );
				m_dlg_log->AddLine( str );
				a->Remove();
			}
			else if( !a->IsClosed() )
			{
				s.LoadStringA(IDS_AreaIsNotClosed);
				str.Format( s, a->UID() );
				m_dlg_log->AddLine( str );
				a->Remove();
			}
		}
		// check all areas in net for self-intersection. CPT2 I like to think that, what with PolygonModified() getting called more often nowadays,
		// these checks will be generally redundant.
		for (CArea *a = ia.First(); a; a = ia.Next())
		{
			int ret = a->ClipPolygon( FALSE, FALSE );
			if( ret == -1 )
			{
				s.LoadStringA(IDS_AreaIsSelfIntersecting);
				str.Format( s, a->UID() );
				m_dlg_log->AddLine( str );
			}
			if( ret == 0 )
			{
				s.LoadStringA(IDS_AreaIsOK);
				str.Format( s, a->UID() );
				m_dlg_log->AddLine( str );
			}
			else if( ret == 1 )
			{
				s.LoadStringA(IDS_AreaIsSelfIntersecting2);
				str.Format( s, a->UID() );
				m_dlg_log->AddLine( str );
			}
		}
		// check all areas in net for intersection. 
		bool mod_a1;
		CIter<CArea> ia1 (&net->areas);
		for (CArea *a1 = ia1.First(); a1; a1 = ia1.Next())
		{
			do {
				CRect b1 = a1->GetCornerBounds();
				mod_a1 = FALSE;
				CIter<CArea> ia2 (&net->areas, a1);
				ia2.Next();														// Advance to polyline AFTER a1.
				for (CArea *a2 = ia2.Next(); a2; a2 = ia2.Next())
				{
					CRect b2 = a2->GetCornerBounds();
					if ( b1.left > b2.right || b1.right < b2.left
							|| b1.bottom > b2.top || b1.top < b2.bottom ) continue;
					int ret = a1->TestIntersection( a2, true );
					if( ret == 1 && (a1->utility == -1 || a2->utility == -1) )
					{
						s.LoadStringA(IDS_AreasIntersectButCantBeCombined);
						str.Format( s, a1->UID(), a2->UID(), a1->UID() );
						m_dlg_log->AddLine( str );
					}
					else if( ret == 1 )
					{
						ret = a1->CombinePolyline( a2 );
						if (ret == 0) continue;
						a2->Remove();												// Virtual func. undraws and detaches area from net->areas
						s.LoadStringA(IDS_AreasIntersectAndHaveBeenCombined);
						str.Format( s, a1->UID(), a2->UID() );
						m_dlg_log->AddLine( str );
						mod_a1 = TRUE;
					}
					else if( ret == 2 )
					{
						s.LoadStringA(IDS_AreasHaveAnIntersectingArc);
						str.Format( s, a1->UID(), a2->UID() );
						m_dlg_log->AddLine( str );
					}
				}
			}
			while (mod_a1);		// if a1 is modified, we need to check it against all the other areas again.
		}
	}

	str.LoadStringA(IDS_Done);
	m_dlg_log->AddLine( str );
	// if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
	//	m_nlist->OptimizeConnections();
	ProjectModified( TRUE );
	m_view->Invalidate( FALSE );
}

void CFreePcbDoc::OnToolsCheckTraces()
{
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	view->Log(LOG_ON_TOOLS_CHECK_TRACES);
	CString str;
	m_view->CancelSelection();
	m_dlg_log->ShowWindow( SW_SHOW );   
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	CString s ((LPCSTR) IDS_CheckingTracesForZeroLengthOrColinearSegments);
	m_dlg_log->AddLine( s );
	m_nlist->CleanUpAllConnections( &str );
	m_dlg_log->AddLine( str );
	s.LoadStringA(IDS_Done);
	m_dlg_log->AddLine(s);
	ResetUndoState();
	Redraw();
}

void CFreePcbDoc::OnEditPasteFromFile()
{
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	CString s ((LPCSTR) IDS_AllFiles);
	CString pathname, filename;
	CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_EXPLORER, 
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );
	s.LoadStringA(IDS_PasteGroupFromFile);
	dlg.m_ofn.lpstrTitle = s; 
	if (!view->m_bReplaying)
	{
		int ret = dlg.DoModal();
		if( ret != IDOK )
			{ m_view->LogCancel(); return; }
		pathname = dlg.GetPathName();
		filename = dlg.GetFileName();
	}
	// Log records the path and file name, but not the contents of the incoming file...  If the need arises, will have to obtain a copy of the
	// file before debugging.
	m_view->Log(LOG_ON_EDIT_PASTE_FROM_FILE);
	m_view->LogXtra( &pathname );
	m_view->LogXtra( &filename );

	ResetUndoState();									// CPT2 TODO ok to omit?
	CStdioFile pcb_file;
	if (!pcb_file.Open( pathname, CFile::modeRead, NULL ))
	{
		CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
		mess.Format( s, pathname );
		AfxMessageBox( mess );
		return;
	}
	// clear clipboard lists to accommodate the incoming file.
	clip_slist->shapes.RemoveAll();
	clip_nlist->nets.RemoveAll();
	clip_plist->parts.RemoveAll();
	clip_tlist->texts.RemoveAll();
	clip_smcutouts.RemoveAll();
	clip_boards.RemoveAll();
	try
	{
		// get layers
		int fpos = 0;
		CString in_str = "";
		while( in_str.Left(16) != "n_copper_layers:" )
		{
			fpos = pcb_file.GetPosition();
			pcb_file.ReadString( in_str );
		}
		int n_copper_layers = atoi( in_str.Right( in_str.GetLength()-16 ) );
		if( n_copper_layers > m_num_copper_layers )
		{
			CString mess ((LPCSTR) IDS_TheGroupFileThatYouArePastingHasMoreLayers);
			AfxMessageBox( mess, MB_OK );
			pcb_file.Close();
			return;
		}

		// CPT2 here's a sleazy way to deal with reading in the file to the clipboard:  transfer the current contents of the main lists
		// (m_slist, m_plist, m_nlist, m_tlist, boards, smcutouts) to temporary locations.  Then read in the file to the main lists in the usual way,
		// transfer from the main lists to the clipboard lists, and restore the original main lists.
		CHeap<CShape> tmpShapes;
		CHeap<CPart> tmpParts;
		CHeap<CNet> tmpNets;
		CHeap<CText> tmpTexts;
		CHeap<CBoard> tmpBoards;
		CHeap<CSmCutout> tmpSms;
		m_slist->shapes.TransferTo(&tmpShapes);
		m_plist->parts.TransferTo(&tmpParts);
		m_nlist->nets.TransferTo(&tmpNets);
		m_tlist->texts.TransferTo(&tmpTexts);
		boards.TransferTo(&tmpBoards);
		smcutouts.TransferTo(&tmpSms);
		m_slist->ReadShapes( &pcb_file );
		ReadBoardOutline( &pcb_file );
		ReadSolderMaskCutouts( &pcb_file );
		m_plist->ReadParts( &pcb_file );
		m_nlist->ReadNets( &pcb_file, m_read_version );
		m_tlist->ReadTexts( &pcb_file );
		pcb_file.Close();
		m_slist->shapes.TransferTo(&clip_slist->shapes);
		m_plist->parts.TransferTo(&clip_plist->parts);
		m_nlist->nets.TransferTo(&clip_nlist->nets);
		m_tlist->texts.TransferTo(&clip_tlist->texts);
		boards.TransferTo(&clip_boards);
		smcutouts.TransferTo(&clip_smcutouts);
		// Better make sure that members of clip_plist and clip_nlist point to their parent lists correctly:
		CIter<CPart> ip (&clip_plist->parts);
		for (CPart *part = ip.First(); part; part = ip.Next())
			part->m_pl = clip_plist;
		CIter<CNet> in (&clip_nlist->nets);
		for (CNet *net = in.First(); net; net = in.Next())
			net->m_nlist = clip_nlist;
		tmpShapes.TransferTo(&m_slist->shapes);
		tmpParts.TransferTo(&m_plist->parts);
		tmpNets.TransferTo(&m_nlist->nets);
		tmpTexts.TransferTo(&m_tlist->texts);
		tmpBoards.TransferTo(&boards);
		tmpSms.TransferTo(&smcutouts);
	}
	catch( CString * err_str )
	{
		// parsing error
		AfxMessageBox( *err_str );
		delete err_str;
		pcb_file.Close();
		return;
	}
	m_view->OnGroupPaste();
}

// Purge footprints from local cache unless they are used in
// partlist or clipboard
//
void CFreePcbDoc::PurgeFootprintCache()
{
	CIter<CShape> is (&m_slist->shapes);
	for (CShape *s = is.First(); s; s = is.Next())
		if( m_plist->GetNumFootprintInstances( s ) == 0 && clip_plist->GetNumFootprintInstances( s ) == 0 )
			s->SaveUndoInfo(),
			m_slist->shapes.Remove(s);
}


void CFreePcbDoc::OnFilePrint()
{
	// TODO: Add your command handler code here
}

void CFreePcbDoc::OnFileExportDsn()
{
	if( m_project_modified )
	{
		CString s ((LPCSTR) IDS_ThisFunctionCreatesADsnFile);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
			OnFileSave();
	}
	CDlgExportDsn dlg;
	CString dsn_filepath = m_pcb_full_path;
	int dot_pos = dsn_filepath.ReverseFind( '.' );
	if( dot_pos != -1 )
		dsn_filepath = dsn_filepath.Left( dot_pos );
	dsn_filepath += ".dsn";
	int num_polys = boards.GetSize();
	if( m_dsn_signals_poly >= num_polys )
		m_dsn_signals_poly = 0;
	if( m_dsn_bounds_poly >= num_polys )
		m_dsn_bounds_poly = 0;
	dlg.Initialize( &dsn_filepath, boards.GetSize(), 
						m_dsn_bounds_poly, m_dsn_signals_poly, m_dsn_flags );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_dlg_log->ShowWindow( SW_SHOW );   
		m_dlg_log->UpdateWindow();
		m_dlg_log->BringWindowToTop();
		m_dlg_log->Clear();
		m_dlg_log->UpdateWindow(); 

		m_dsn_flags = dlg.m_flags;
		m_dsn_bounds_poly = dlg.m_bounds_poly;
		m_dsn_signals_poly = dlg.m_signals_poly;
		OnFileSave();
		CString s ((LPCSTR) IDS_SavingProjectFile), mess;
		mess.Format(s, m_pcb_full_path);
		m_dlg_log->AddLine( mess );
		s.LoadStringA(IDS_CreatingDsnFile);
		mess.Format(s, dsn_filepath);
		m_dlg_log->AddLine( mess );  

		CString commandLine = "\"" + m_app_dir + "\\fpcroute.exe\"";
		int from_to = m_dsn_flags & CDlgExportDsn::DSN_FROM_TO_MASK;
		if( from_to == CDlgExportDsn::DSN_FROM_TO_ALL )
			commandLine += " -F3";
		else if( from_to == CDlgExportDsn::DSN_FROM_TO_LOCKED )
			commandLine += " -F1";
		else if( from_to == CDlgExportDsn::DSN_FROM_TO_NET_LOCKED )
			commandLine += " -F2";
		if( m_dsn_flags & CDlgExportDsn::DSN_VERBOSE )
			commandLine += " -V"; 
		if( m_dsn_flags & CDlgExportDsn::DSN_INFO_ONLY )
			commandLine += " -I";
		if( m_dsn_bounds_poly != 0 || m_dsn_signals_poly != 0 ) 
		{
			CString str;
			str.Format( " -U%d,%d", m_dsn_bounds_poly+1, m_dsn_signals_poly+1 );
			commandLine += str;
		}
		commandLine += " \"" + m_pcb_full_path + "\"";
//		CString commandLine = "C:/freepcb/bin/RTconsole.exe  C:/freepcb/bin/fpcroute.exe -V C:/freepcb/bin/test";
		s.LoadStringA(IDS_Run);
		mess.Format(s, commandLine);
		m_dlg_log->AddLine( mess );  
		RunConsoleProcess( commandLine, m_dlg_log );
#if 0
		HANDLE hOutput, hProcess;
		hProcess = SpawnAndRedirect(commandLine, &hOutput, NULL); 
		if (!hProcess) 
		{
			m_dlg_log->AddLine( "Failed!\r\n" );
			return;
		}

		// if necessary, this could be put in a separate thread so the GUI thread is not blocked
		BeginWaitCursor();
		CHAR buffer[65];
		DWORD read;
		while (ReadFile(hOutput, buffer, 64, &read, NULL))
		{
			buffer[read] = '\0';
			m_dlg_log->AddLine( buffer );
		}
		CloseHandle(hOutput);
		CloseHandle(hProcess);
		EndWaitCursor();
#endif
	}
}

void CFreePcbDoc::OnFileImportSes()
{
	CDlgImportSes dlg;
	dlg.Initialize( &m_ses_full_path, &m_pcb_full_path );
	int ret = dlg.DoModal(); 
	if( ret != IDOK )
		return;
	m_dlg_log->ShowWindow( SW_SHOW );   
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop(); 
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow(); 
	// save current project if modified (including dialog parameters)
	if( dlg.m_ses_filepath != m_ses_full_path )
	{
		m_ses_full_path = dlg.m_ses_filepath;
		ProjectModified( TRUE );
	}
	if( m_project_modified )
	{
		CString s ((LPCSTR) IDS_ProjectModifiedSaveBeforeImport);
		int ret = AfxMessageBox( s, MB_YESNO );
		if( ret = IDYES )
		{
			OnFileSave();
		}
		else
			ProjectModified( FALSE );
	}
	CString temp_file_name = "~temp$$$.fpc";   
	CString temp_routed_file_name = "~temp$$$_routed.fpc";
	CString temp_file_path = m_path_to_folder + "\\" + temp_file_name;
	CString temp_routed_file_path = m_path_to_folder + "\\" + temp_routed_file_name;
	struct _stat buf;
	int err = _stat( temp_file_path, &buf );
	if( !err )
	{
		CString s ((LPCSTR) IDS_Delete2), mess;
		mess.Format(s, temp_file_path);
		m_dlg_log->AddLine( mess );  
		remove( temp_file_path );
	}
	err = _stat( dlg.m_routed_pcb_filepath, &buf );
	if( !err )
	{
		CString s ((LPCSTR) IDS_Delete2), mess;
		mess.Format(s, dlg.m_routed_pcb_filepath);
		m_dlg_log->AddLine( mess );  
		remove( dlg.m_routed_pcb_filepath );
	}
	m_ses_full_path = dlg.m_ses_filepath;

	// save project as temporary file
	CString s ((LPCSTR) IDS_Save), mess;
	mess.Format(s, temp_file_path);
	m_dlg_log->AddLine( mess );
	CString old_pcb_filename = m_pcb_filename;
	CString old_pcb_full_path = m_pcb_full_path;
	m_pcb_filename = temp_file_name;
	m_pcb_full_path = temp_file_path;
	OnFileSave();

	// import session file
	CString verbose = "";
	if( dlg.m_bVerbose )
		verbose = "-V ";
	CString commandLine = "\"" + m_app_dir + "\\fpcroute.exe\" -B " + verbose + "\"" +
		temp_file_path + "\" \"" + m_ses_full_path + "\""; 
	s.LoadStringA(IDS_Run);
	mess.Format(s, commandLine);
	m_dlg_log->AddLine( mess );  
	RunConsoleProcess( commandLine, m_dlg_log );
	err = _stat( temp_routed_file_path, &buf );
	if( err )
	{
		s.LoadStringA(IDS_FpcRouteFailedToCreate);
		mess.Format(s, temp_routed_file_path);
		m_dlg_log->AddLine( mess );  
		return;
	}
	s.LoadStringA(IDS_Rename);
	mess.Format(s, temp_routed_file_path, dlg.m_routed_pcb_filepath);
	m_dlg_log->AddLine( mess );  
	err = rename( temp_routed_file_path, dlg.m_routed_pcb_filepath ); 
	if( err )
	{
		s.LoadStringA(IDS_RenamingProjectFileFailed);
		mess.Format(s, temp_routed_file_path, dlg.m_routed_pcb_filepath); 
		m_dlg_log->AddLine( mess );
	}
	CString old_ses_full_path = m_ses_full_path;
	s.LoadStringA(IDS_Load);
	mess.Format(s, dlg.m_routed_pcb_filepath);
	m_dlg_log->AddLine( mess );  
	OnFileAutoOpen( dlg.m_routed_pcb_filepath );
	m_ses_full_path = old_ses_full_path;
	s.LoadStringA(IDS_Reimport);
	mess.Format(s, m_ses_full_path);
	m_dlg_log->AddLine( mess );  
	ImportSessionFile( &m_ses_full_path, m_dlg_log, dlg.m_bVerbose );
	s.LoadStringA(IDS_Done);
	m_dlg_log->AddLine( s );  
	ProjectModified( TRUE );
	ResetUndoState();
}

void CFreePcbDoc::ResetUndoState()
{
	for (int i=0; i<m_undo_items.GetSize(); i++)
		delete m_undo_items[i];
	m_undo_items.RemoveAll();
	for (int i=0; i<m_undo_records.GetSize(); i++)
		delete m_undo_records[i];
	m_undo_records.RemoveAll();
	m_undo_pos = 0;
	m_undo_last_uid = CPcbItem::GetNextUid();
}

void CFreePcbDoc::FinishUndoRecord(bool bCombineWithPrevious)
{
	// The user operation has just finished.  Create a bundle containing all of the undo items that have been created by CPcbItem::SaveUndoInfo().
	// We have to clear duplicates out of that CArray (unlike with CHeap's, that's not automatic).
	// If bCombineWithPrevious is true, we combine this operation's undo items with the items in the previously created undo record, so that
	//   hitting ctrl-z jumps user all the way back to the state before this routine's previous invocation (useful e.g. with arrow keys).
	// "uitems" will hold the culled-out list of items, possibly merged with the previous record's items:
	CArray<CUndoItem*> uitems;
	items.SetUtility(0);
	if (m_undo_pos==0)
		bCombineWithPrevious = false;
	if (bCombineWithPrevious)
	{
		CUndoRecord *rec = m_undo_records[m_undo_pos-1];
		for (int i=0; i<rec->nItems; i++)
		{
			CUndoItem *uitem = rec->items[i];
			CPcbItem *item = CPcbItem::FindByUid( uitem->UID() );
			uitems.Add(uitem);
			// Set the utility of "item" so that another CUndoItem for it (from m_undo_items) won't be added to uitems in the next loop.
			// It's _just_ possible that "item" will be null because the object got removed and then garbage-collected (e.g. with a group
			//  selected, user is hitting arrow keys repeatedly, and an area in the group gets merged into some other area).  If so, we
			//  can be confident that no CUndoItem for the object is in m_undo_items, because SaveUndoInfo() won't have been invoked on a
			//  garbage-collected item.  Therefore don't worry about the utility bit in that case:
			if (item)
				item->utility = 1;
		}
	}

	for (int i=0; i<m_undo_items.GetSize(); i++) 
	{
		CUndoItem *uitem = m_undo_items[i];
		int uid = uitem->UID();
		CPcbItem *item = CPcbItem::FindByUid( uid );
		ASSERT(item);
		item->bUndoInfoSaved = false;						// For next time...
		if (item->utility)
			delete uitem;
		else if (uid < m_undo_last_uid)
			uitems.Add(uitem),
			item->utility = 1;
		else
			// It can happen that SaveUndoInfo() has been called on items that were created since the previous user op ended.  We will add entries
			// for these to uitems, but later...
			;
	}
	// Determine which objects have been created since the last time this routine was invoked.  They will get added to the record as cundo_items with
	// the bWasCreated bit set.
	int next_uid = CPcbItem::GetNextUid();
	for (int i = m_undo_last_uid; i < next_uid; i++)
	{
		// Calling IsOnPcb() ensures, for instance, that objects created on the clipboard are left out of account.  Also this way we
		// ignore the substantial number of temporary cshapes that may get created (during the fp editor or the DlgAddPart, say).
		CPcbItem *item = CPcbItem::FindByUid(i);
		ASSERT(item);
		if (item->IsOnPcb() && !item->IsDRE())
			uitems.Add( new CUndoItem(this, i, true) );
	}
	m_undo_last_uid = next_uid;					// For next time...
	m_undo_items.RemoveAll();
	if (uitems.GetSize()==0)
		return;
	// Create the record!
	CUndoRecord *rec = new CUndoRecord (&uitems, &m_view->m_sel);
	// Clear out any redo records from m_undo_records (those at or beyond position m_undo_pos), then add "rec" to the end of the reduced list.
	if (bCombineWithPrevious)
		m_undo_pos--,
		m_undo_records[m_undo_pos]->nItems = 0;						// So that "delete m_undo_records[m_undo_pos]" below doesn't clobber the undo-items.
	for (int i=m_undo_pos; i < m_undo_records.GetSize(); i++)
		delete m_undo_records[i];
	m_undo_records.SetSize( m_undo_pos );
	if (m_undo_pos >= UNDO_MAX)
		delete m_undo_records[0],
		m_undo_records.RemoveAt(0),
		m_undo_pos--;
	m_undo_records.Add( rec );
	m_undo_pos++;
}

void CFreePcbDoc::CreateMoveOriginUndoRecord( int dx, int dy )
{
	CUndoRecord *rec = new CUndoRecord (dx, dy, &m_view->m_sel);	// Theoretically we should have CUndoRecordMoveOrigin as a subclass of
																	// CUndoRecord, but screw it...
	// Add record to the undo list just as in FinishUndoRecord
	for (int i=m_undo_pos; i < m_undo_records.GetSize(); i++)
		delete m_undo_records[i];
	m_undo_records.SetSize( m_undo_pos );
	if (m_undo_pos >= UNDO_MAX)
		delete m_undo_records[0],
		m_undo_records.RemoveAt(0),
		m_undo_pos--;
	m_undo_records.Add( rec );
	m_undo_pos++;
}

void CFreePcbDoc::OnEditUndo()
{
	// CPT2 converted to the new system.  Routine hands off almost immediately to CUndoRecord::Execute()
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	view->Log(LOG_ON_EDIT_UNDO);
	if (m_undo_items.GetSize() > 0)
		// Maybe the programmer forgot to call FinishUndoRecord()???  Finish up the undo record now anyway
		FinishUndoRecord();
	if (m_undo_pos <= 0) 
		// Silent failure
		return;
	view->CancelHighlight();
	CUndoRecord *rec = m_undo_records[m_undo_pos-1];
	bool bMoveOrigin = rec->Execute( CUndoRecord::OP_UNDO );
	// On exit, rec will have been converted to a redo record.
	m_undo_pos--;
	ProjectModified(true);
	if (bMoveOrigin)
		view->OnViewAllElements();
	view->HighlightSelection();
}

void CFreePcbDoc::OnEditRedo()
{
	CFreePcbView * view = m_view;
	if (view->m_bReplayMode && !view->m_bReplaying)
		return;
	view->Log(LOG_ON_EDIT_REDO);
	if (m_undo_pos >= m_undo_records.GetSize())
		return;
	view->CancelHighlight();
	CUndoRecord *rec = m_undo_records[m_undo_pos];
	bool bMoveOrigin = rec->Execute( CUndoRecord::OP_REDO );
	// On exit, rec will have been converted back to an undo record.
	m_undo_pos++;
	ProjectModified(true);
	if (bMoveOrigin)
		view->OnViewAllElements();
	view->HighlightSelection();
}

void CFreePcbDoc::UndoNoRedo()
{
	// CPT2.  Similar to OnEditUndo(), but used only occasionally, e.g. when user aborts a just-started operation like dragging a stub.
	if (m_undo_items.GetSize() > 0)
		FinishUndoRecord();
	if (m_undo_pos <= 0) 
		return;
	m_view->CancelSelection();
	CUndoRecord *rec = m_undo_records[m_undo_pos-1];
	rec->Execute( CUndoRecord::OP_UNDO_NO_REDO );
	// rec is unchanged on exit, but is now garbage.
	delete rec;
	m_undo_records.RemoveAt(m_undo_pos-1);
	m_undo_pos--;
	ProjectModified(true);
}


void CFreePcbDoc::OnFileGenerateReportFile()
{
	CDlgReport dlg;
	dlg.Initialize( this ); 
	int ret = dlg.DoModal();
	if( ret = IDOK )
		m_report_flags = dlg.m_flags;	// update flags
}

void CFreePcbDoc::OnFileLoadLibrary()
{
	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();
	InitializeNewProject();		// set defaults

	// get project file name
	CString s ((LPCSTR) IDS_LibraryFiles);
	CFileDialog dlg( 1, "fpl", LPCTSTR(m_pcb_filename), 0, 
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );
	dlg.AssertValid();

	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_parent_folder;

	if( m_full_lib_dir != "" )
		dlg.m_ofn.lpstrInitialDir = m_lib_dir;

	// now show dialog
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		// read project file
		CString pathname = dlg.GetPathName();
		FileLoadLibrary( pathname );
	}
	else
	{
		// CANCEL or error
		DWORD dwError = ::CommDlgExtendedError();
		if( dwError )
		{
			CString str, s ((LPCSTR) IDS_FileOpenDialogErrorCode);
			str.Format( s, (unsigned long)dwError );
			AfxMessageBox( str );
		}
	}
}
void CFreePcbDoc::FileLoadLibrary( LPCTSTR pathname )
{
	// CPT2 This is associated with a menu item that is permanently gray, it appears.  I'll think about re-implementing later...
#ifndef CPT2
	BOOL bOK = FileOpen( pathname, TRUE );
	if( bOK )
	{
		// now add one instance of each footprint in cache
		POSITION pos;
		CString key;
		void * ptr;
		LPCSTR p;
		CShape *shape;
		CString ref, val = "";
		int i=1, x=0, y=0, max_height=0;
		for( pos = m_footprint_cache_map.GetStartPosition(); pos != NULL; )
		{
			m_footprint_cache_map.GetNextAssoc( pos, key, ptr );
			p = (LPCSTR)key;
			shape = (CShape*)ptr;
			ref.Format( "LIB_%d", i );
			CPart * part = m_plist->Add( shape, &ref, &val, NULL, x, y, 0, 0, 1, 0 );
			// get bounding rectangle of pads and outline
			CRect shape_r = part->shape->GetBounds();
			int height = shape_r.top - shape_r.bottom;
			int width = shape_r.right - shape_r.left;
			// get dimensions of bounding rectangle for value text
			int refStroke = part->m_ref->m_stroke_width, refFontSz = part->m_ref->m_font_size;
			part->SetValue( &shape->m_name,
				shape_r.left, shape_r.top + refStroke, 0, 
				refFontSz, refStroke, 1, LAY_SILK_TOP );
			CRect vr = part->m_value->GetRect();
			int value_width = vr.right - vr.left;
			// see if we can fit part between x and 8 inches
			int max_width = max( width, value_width );
			BOOL bFits = (x + max_width) < (8000*NM_PER_MIL);
			if( !bFits ) 
			{
				// start new row
				x = 0;
				y -= max_height;
				max_height = 0;
			}
			// move part so upper-left corner is at x,y
			part->Move( x - shape_r.left, y - shape_r.top - 2*refFontSz, 0, 0 );
			// make ref invisible
			part->m_ref->m_bShown = false;
			// move value to top of part
			part->SetValue( &shape->m_name, 
				shape_r.left, shape_r.top + refStroke, 0, 
				refFontSz, refStroke, 1, part->m_ref->m_layer );
			part->Draw( );
			i++;
			x += max_width + 200*NM_PER_MIL;	// step right .2 inches
			max_height = max( max_height, height + 2*refFontSz );
		}
		if( m_pcb_filename.Right(4) == ".fpl" )
		{
			int len = m_pcb_filename.GetLength();
			m_pcb_filename.SetAt(len-1, 'c');
			len = m_pcb_full_path.GetLength();
			m_pcb_full_path.SetAt(len-1, 'c');
		}
		else
		{
			m_pcb_filename = m_pcb_filename + ".fpc";
			m_pcb_full_path = *pathname + ".fpc";
		}
		CString s ((LPCSTR) IDS_FreePCBLibraryProject);
		m_window_title = s + m_pcb_filename;
		CWnd* pMain = AfxGetMainWnd();
		pMain->SetWindowText( m_window_title );
		m_view->OnViewAllElements();
		ProjectModified( FALSE );
	}
#endif
}

void CFreePcbDoc::OnFileSaveLibrary()
{
	// CPT2 TODO this has never been fully implemented (menu item is always grayed out).
	CDlgSaveLib dlg;
	CArray<CString> names;
	CIter<CPart> ip (&m_plist->parts);
	for (CPart *part = ip.First(); part; part = ip.Next())
		names.Add( part->ref_des );								// CPT2 was "part->value" in the old code, which I can't imagine was right???
	dlg.Initialize( &names );
	int ret = dlg.DoModal();
}


// CPT (all that follows)

void ReadFileLines(CString &fname, CArray<CString> &lines) 
{
	// Helper function used when making modifications to default.cfg.  Bug fix #28
	lines.RemoveAll();
	CStdioFile file;
	CString line;
	int ok = file.Open( LPCSTR(fname), CFile::modeRead | CFile::typeText, NULL );
	if (!ok) return;
	while (1)	
	{
		if (!file.ReadString( line )) break;
		line.Trim(),
		lines.Add(line + "\n");
	}
	file.Close();
}

void WriteFileLines(CString &fname, CArray<CString> &lines) 
{
	// Helper function used when making modifications to default.cfg
	CStdioFile file;
	int ok = file.Open( LPCSTR(fname), CFile::modeCreate | CFile::modeWrite | CFile::typeText, NULL );
	if (!ok) return;
	for (int i=0; i<lines.GetSize(); i++)
		file.WriteString(lines[i]);
	file.Close();
}

void ReplaceLines(CArray<CString> &oldLines, CArray<CString> &newLines, char *key) 
{
	// Another helper for making modifications to default.cfg. Look through "oldLines" and eliminate all entries beginning with "key" (if any).
	// Then append to oldLines all members of newLines beginning with that key.  Must ensure that the [end] line is still at the end...
	int keyLgth = strlen(key);
	for (int i=0; i<oldLines.GetSize(); i++) 
	{
		CString str = oldLines[i].TrimLeft();
		if (str.Left(keyLgth) == key || str.Left(5) == "[end]")
			oldLines.RemoveAt(i),
			i--;
	}
	if (oldLines.GetSize()==0)
		oldLines.Add("[options]\n");
	for (int i=0; i<newLines.GetSize(); i++) 
	{
		CString str = newLines[i].TrimLeft();
		if (str.Left(keyLgth) == key)
			oldLines.Add(str);
	}
	oldLines.Add("[end]\n");
}


void CFreePcbDoc::OnToolsPreferences() 
{
	CDlgPrefs dlg;
	dlg.Init( m_bReversePgupPgdn, m_bLefthanded, m_bHighlightNet, m_bDragNoSides, m_bErrorSound, m_auto_interval, 
		m_auto_ratline_disable, m_auto_ratline_min_pins); 
	int ret = dlg.DoModal();
	if( ret != IDOK ) 
		return;
	m_bReversePgupPgdn = dlg.m_bReverse;
	m_bLefthanded = dlg.m_bLefthanded;
	m_bHighlightNet = dlg.m_bHighlightNet;
	m_bDragNoSides = dlg.m_bDragNoSides;
	m_bErrorSound = dlg.m_bErrorSound;
	m_auto_interval = dlg.m_auto_interval;
	m_auto_ratline_disable = dlg.m_bAuto_Ratline_Disable;
	m_auto_ratline_min_pins = dlg.m_auto_ratline_min_pins;
	if (dlg.m_bReinstateWarnings)
		for (int i=0; i<NUM_WARNINGS; i++)
			m_bWarningDisable[i] = false;

	// CPT2.  Deal with changes to default.cfg directory.  This feature became necessary when good 'old MS decided with Win7 to write-protect 
	// everything within the C:\Program Files folder.  Assumes that CDlgPrefs already did a check on the writeability of the new dir.
	CString oldDir = m_defaultcfg_dir;
	CString newDir = dlg.m_defaultcfg_dir;
	CString oldFn = oldDir + "\\default.cfg";
	CString newFn = newDir + "\\default.cfg";
	m_defaultcfg_dir = newDir;
	theApp.WriteProfileStringA(_T("Settings"),_T("DefaultCfgDir"), newDir);
	
	// Save other values to default.cfg
	CString line;
	CArray<CString> oldLines, newLines;
	line.Format( "autosave_interval: \"%d\"\n", m_auto_interval );
	newLines.Add( line );
	line.Format( "auto_ratline_disable: \"%d\"\n", m_auto_ratline_disable );
	newLines.Add( line );
	line.Format( "auto_ratline_disable_min_pins: \"%d\"\n", m_auto_ratline_min_pins );
	newLines.Add( line );
	line.Format( "reverse_pgup_pgdn: \"%d\"\n", m_bReversePgupPgdn);
	newLines.Add( line );
	line.Format( "lefthanded_mode: \"%d\"\n", m_bLefthanded);
	newLines.Add( line );
	line.Format( "highlight_net: \"%d\"\n", m_bHighlightNet);
	newLines.Add( line );
	line.Format( "drag_no_sides: \"%d\"\n", m_bDragNoSides);
	newLines.Add( line );
	line.Format( "error_sound: \"%d\"\n", m_bErrorSound);
	newLines.Add( line );
	for (int i=0; i<NUM_WARNINGS; i++)
		line.Format( "warning_disable: %d %d\n", i, m_bWarningDisable[i]),
		newLines.Add(line);
	ReadFileLines(oldFn, oldLines);
	ReplaceLines(oldLines, newLines, "autosave_interval");
	ReplaceLines(oldLines, newLines, "auto_ratline_disable");
	ReplaceLines(oldLines, newLines, "auto_ratline_disable_min_pins");
	ReplaceLines(oldLines, newLines, "reverse_pgup_pgdn");
	ReplaceLines(oldLines, newLines, "lefthanded_mode");
	ReplaceLines(oldLines, newLines, "highlight_net");
	ReplaceLines(oldLines, newLines, "drag_no_sides");
	ReplaceLines(oldLines, newLines, "error_sound");
	ReplaceLines(oldLines, newLines, "warning_disable");
	WriteFileLines(newFn, oldLines);
	m_view->SetFKText(m_view->m_cursor_mode);					// In case user changed the left-handed mode...
}

void CFreePcbDoc::OnViewRoutingGrid() 
{
	CDlgGridVals dlg (&m_routing_grid, &m_routing_grid_hidden, IDS_EditRoutingGridValues);
	int ret = dlg.DoModal();
	if( ret != IDOK ) 
		return;
	// CDlgGridVals::DoDataExchange() already updated m_routing_grid and m_routing_grid_hidden.  We just need to change the
	// ToolBar lists:
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
		m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, m_view->m_units );
	ProjectModified(true);
	if (dlg.bSetDefault) 
	{
		CArray<CString> oldLines, newLines;
		CString fn = m_defaultcfg_dir + "\\" + "default.cfg";
		ReadFileLines(fn, oldLines);
		CollectOptionsStrings(newLines);
		ReplaceLines(oldLines, newLines, "routing_grid_item");
		ReplaceLines(oldLines, newLines, "routing_grid_hidden");
		WriteFileLines(fn, oldLines);
	}
}

void CFreePcbDoc::OnViewPlacementGrid() 
{
	CDlgGridVals dlg (&m_part_grid, &m_part_grid_hidden, IDS_EditPlacementGridValues);
	int ret = dlg.DoModal();
	if( ret != IDOK ) 
		return;
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
		m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, m_view->m_units );
	ProjectModified(true);
	if (dlg.bSetDefault) 
	{
		CArray<CString> oldLines, newLines;
		CString fn = m_defaultcfg_dir + "\\" + "default.cfg";
		ReadFileLines(fn, oldLines);
		CollectOptionsStrings(newLines);
		ReplaceLines(oldLines, newLines, "placement_grid_item");
		ReplaceLines(oldLines, newLines, "placement_grid_hidden");
		WriteFileLines(fn, oldLines);
	}
}

void CFreePcbDoc::OnViewVisibleGrid() {
	CDlgGridVals dlg (&m_visible_grid, &m_visible_grid_hidden, IDS_EditVisibleGridValues);
	int ret = dlg.DoModal();
	if( ret != IDOK ) 
		return;
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
		m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, m_view->m_units );
	ProjectModified(true);
	if (dlg.bSetDefault) 
	{
		CArray<CString> oldLines, newLines;
		CString fn = m_defaultcfg_dir + "\\" + "default.cfg";
		ReadFileLines(fn, oldLines);
		CollectOptionsStrings(newLines);
		ReplaceLines(oldLines, newLines, "visible_grid_item");
		ReplaceLines(oldLines, newLines, "visible_grid_hidden");
		WriteFileLines(fn, oldLines);
	}
}

// end CPT

void CFreePcbDoc::Redraw() 
{
	// CPT2 r313, latest-n-greatest system for drawing and redrawing.  An outer-level routine like CFreePcbView::OnLButtonUp() should call this
	// routine after all the modifications of the CPcbItem hierarchy requested by user have been completed.  At that time, this routine 
	// runs through CHeap this->redraw, a list that has been built up during the course of the modifications by routine CPcbItem::MustRedraw().
	// After double-checking that each item is still valid, routine invokes Draw() on each one.  At the end redraw gets cleared out.
	// Note that this routine is used only in the main editor.
	CIter<CPcbItem> ii (&redraw);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (i->IsOnPcb() && !i->IsFootItem())
			i->Draw();
	redraw.RemoveAll();
}


void CFreePcbDoc::GarbageCollect() {
	// CPT2 Part of my new architecture is to take some of the hassle out of memory management for pcb-items by using garbage collection.
	// Each time an item is constructed, it is added to the "items" array.  If an item goes out of use, one does not have to "delete" it;
	// just unhook it from its parent entity or array.  When garbage collection time comes, we'll first clear the utility bits on all members of
	// "items".  Then we'll scan through the doc's netlist (recursing through connects, segs, vtxs, tees, areas), partlist (recursing through pins), 
	// textlist, smcutout-list, board-list and shape-list, marking utility bits as we go.  Also do the same for the clipboard lists.
	// Also dre items!  (Almost forgot)
	// At the end, items with clear utility bits can be deleted.
	items.SetUtility(0);

	CIter<CNet> in (&m_nlist->nets);
	for (CNet *n = in.First(); n; n = in.Next()) 
		n->MarkConstituents(1);
	CIter<CPart> ip (&m_plist->parts);
	for (CPart *p = ip.First(); p; p = ip.Next())
		p->MarkConstituents(1);
	m_tlist->texts.SetUtility(1);
	CIter<CSmCutout> ism (&smcutouts);
	for (CSmCutout *sm = ism.First(); sm; sm = ism.Next())
		sm->MarkConstituents(1);
	CIter<CBoard> ib (&boards);
	for (CBoard *b = ib.First(); b; b = ib.Next())
		b->MarkConstituents(1);
	CIter<CShape> is (&m_slist->shapes);
	for (CShape *s = is.First(); s; s = is.Next())
		s->MarkConstituents(1);

	CIter<CNet> in2 (&clip_nlist->nets);
	for (CNet *n2 = in2.First(); n2; n2 = in2.Next()) 
		n2->MarkConstituents(1);
	CIter<CPart> ip2 (&clip_plist->parts);
	for (CPart *p2 = ip2.First(); p2; p2 = ip2.Next())
		p2->MarkConstituents(1);
	clip_tlist->texts.SetUtility(1);
	CIter<CSmCutout> ism2 (&clip_smcutouts);
	for (CSmCutout *sm2 = ism2.First(); sm2; sm2 = ism2.Next())
		sm2->MarkConstituents(1);
	CIter<CBoard> ib2 (&clip_boards);
	for (CBoard *b2 = ib2.First(); b2; b2 = ib2.Next())
		b2->MarkConstituents(1);
	CIter<CShape> is2 (&clip_slist->shapes);
	for (CShape *s2 = is2.First(); s2; s2 = is2.Next())
		s2->MarkConstituents(1);

	m_drelist->dres.SetUtility(1);

	// Do the deletions of unused items!
	int dbgTotal = items.GetSize();
	int dbgDeleted = 0;
	CIter<CPcbItem> ii (&items);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (!i->utility)
			delete i,
			dbgDeleted++;
	// CPT2 TODO consider growing the uid hash table if necessary.
	int dbgRemaining = dbgTotal-dbgDeleted;
	}


// Design rule check.  CPT2:  was in CPartList, now here.
//
void CFreePcbDoc::DRC( int units, DesignRules * dr )
{
	// CPT2.  Formerly dre's we're assigned index numbers as soon as they were generated, and were printed to the log immediately too.
	// Now, though, I'm sorting dre's in a geometrical way (see CDreList::Sort()) after they're created, and so the index #'s will only
	// be generated after the sort.  And the detailed list on the log must be delayed till then also.  Accordingly, when dre's are first created, 
	// I'll just send a "." to the log, plus periodic reports of the #'s of errors.
	CString d_str, x_str, y_str;
	CString str, str2;
	CDlgLog *log = m_dlg_log;
	int num_layers = LAY_TOP_COPPER + m_num_copper_layers;
	m_drelist->Clear();

	// iterate through parts, setting DRC params and
	// checking for following errors:
	//	RING_PAD	
	//	BOARDEDGE_PAD
	//	BOARDEDGE_PADHOLE
	//	COPPERGRAPHIC_PAD
	//	COPPERGRAPHIC_PADHOLE
	str.LoadString( IDS_CheckingParts );
	if( log )
		log->AddLine( str );
	CIter<CPart> ip (&m_plist->parts);
	for (CPart *part = ip.First(); part; part = ip.Next())
	{
		if (!part->shape) continue;
		// set DRC params for part
		// i.e. bounding rectangle and layer mask.  GetDRCInfo() also sets these params for each pin.
		part->GetDRCInfo();

		// Check for errors involving each pin in turn
		CIter<CPin> ipin (&part->pins);
		for (CPin *pin = ipin.First(); pin; pin = ipin.Next())
			DRCPin(pin, units, dr);
	}

	// iterate through parts again, checking against all other parts
	// check for following errors:
	//	PADHOLE_PADHOLE
	//	PAD_PADHOLE
	//	PAD_PAD
	//	COPPERGRAPHIC_PAD,			
	//	COPPERGRAPHIC_PADHOLE,
	// CPT2 to ensure that no duplicate tests occur, only compare part1 with p2 if part1 has the lesser UID
	CIter<CPart> ipart1 (&m_plist->parts);
	CIter<CPart> ipart2 (&m_plist->parts);
	for (CPart *part1 = ipart1.First(); part1; part1 = ipart1.Next())
		for (CPart *part2 = ipart2.First(); part2; part2 = ipart2.Next())
		{
			if (!part1->shape || !part2->shape) continue;
			if (part1->UID() >= part2->UID()) continue;
			// now see if part1 and part2 elements might intersect
			// get max. clearance violation
			int clr = max( dr->pad_pad, dr->hole_copper );
			clr = max( clr, dr->hole_hole );
			clr = max( clr, dr->copper_copper );
			// now check for clearance of rectangles
			if( part1->min_x - part2->max_x > clr )
				continue;	
			if( part2->min_x - part1->max_x > clr )
				continue;
			if( part1->min_y - part2->max_y > clr )
				continue;
			if( part2->min_y - part1->max_y > clr )
				continue;
			// If parts have no elements on common layers and no holes, we can skip comparing 'em
			if( !(part1->layers & part2->layers) )
				if( !part1->hole_flag && !part2->hole_flag ) 
					continue;

			// we need to test elements in these parts.  Compare pins with copper-graphics, then pins with pins.
			DRCPinsAndCopperGraphics(part1, part2, units, dr);
			DRCPinsAndCopperGraphics(part2, part1, units, dr);
			CIter<CPin> ipin1 (&part1->pins);
			CIter<CPin> ipin2 (&part2->pins);
			for (CPin *pin1 = ipin1.First(); pin1; pin1 = ipin1.Next())
				for (CPin *pin2 = ipin2.First(); pin2; pin2 = ipin2.Next())
					DRCPinAndPin( pin1, pin2, units, dr, clr );
		}

	// iterate through all nets
	// check for following errors:
	//	BOARDEDGE_COPPERAREA
	//	TRACE_WIDTH
	//	BOARDEDGE_SEG
	//	RING_VIA
	//	BOARDEDGE_VIA
	//	BOARDEDGE_VIAHOLE
	//	SEG_PAD
	//	VIA_PAD
	//  SEG_COPPERGRAPHIC
	//  VIA_COPPERGRAPHIC
	if( log )
		str.LoadStringA( IDS_Errors ),
		str2.Format(str, m_drelist->GetSize()),
		log->AddLine( str2 );
		str.LoadStringA( IDS_CheckingNetsAndParts ),
		log->AddLine( str );
	int nErrorsLast = m_drelist->GetSize();

	CIter<CNet> in (&m_nlist->nets);
	for (CNet *net = in.First(); net; net = in.Next())
	{
		CIter<CArea> ia (&net->areas);
		for (CArea *a = ia.First(); a; a = ia.Next())
			DRCArea( a, units, dr );
		CIter<CConnect> ic (&net->connects);
		for (CConnect *c = ic.First(); c; c = ic.Next())
			DRCConnect(c, units, dr);
	}

	// now check nets against other nets
	// check for following errors:
	//	SEG_SEG
	//	SEG_VIA
	//	SEG_VIAHOLE
	//	VIA_VIA
	//	VIA_VIAHOLE
	//	VIAHOLE_VIAHOLE
	//	COPPERAREA_INSIDE_COPPERAREA
	//	COPPERAREA_COPPERAREA
	if( log )
		str.LoadStringA( IDS_Errors ),
		str2.Format(str, m_drelist->GetSize() - nErrorsLast),
		log->AddLine( str2 ),
		str.LoadStringA( IDS_CheckingNets2 ),
		log->AddLine( str );
	nErrorsLast = m_drelist->GetSize();

	// get max clearance
	int clr = max( dr->hole_copper, dr->hole_hole );
	clr = max( clr, dr->trace_trace );
	// iterate through all nets.  CPT2 reduce redundancy by only comparing net1 with net2 when net1 has the lesser UID
	CIter<CNet> in1 (&m_nlist->nets);
	CIter<CNet> in2 (&m_nlist->nets);
	for (CNet *net1 = in1.First(); net1; net1 = in1.Next())
		for (CNet *net2 = in2.First(); net2; net2 = in2.Next())
		{
			if (net1->UID() >= net2->UID())
				continue;
			CIter<CConnect> ic1 (&net1->connects);
			CIter<CConnect> ic2 (&net2->connects);
			for (CConnect *c1 = ic1.First(); c1; c1 = ic1.Next())
				for (CConnect *c2 = ic2.First(); c2; c2 = ic2.Next())
					DRCConnectAndConnect(c1, c2, units, dr, clr);

			// now test areas in net1 versus areas in net2
			CIter<CArea> ia1 (&net1->areas);
			CIter<CArea> ia2 (&net2->areas);
			for (CArea *a1 = ia1.First(); a1; a1 = ia1.Next())
				for (CArea *a2 = ia2.First(); a2; a2 = ia2.Next())
					DRCAreaAndArea(a1, a2, units, dr);
		}

	// now check for unrouted connections, if requested
	if( dr->bCheckUnrouted )
		DRCUnrouted(units);

	if (log)
		str.LoadStringA( IDS_Errors ),
		str2.Format(str, m_drelist->GetSize() - nErrorsLast),
		log->AddLine( str2 ),
		str.LoadStringA(IDS_Done4),
		log->AddLine( str );

	m_drelist->Sort();
	// Now that we're sorted, output the complete list to the log
	if (log && m_drelist->GetSize() > 0)
	{
		str.LoadStringA( IDS_CompleteList );
		log->AddLine(str);
		for (CDre *dre = m_drelist->head; dre; dre = dre->next)
			log->AddLine(dre->str);
	}
}

void CFreePcbDoc::DRCPin( CPin *pin, int units, DesignRules *dr )
{
	// CPT2 new helper for DRC.  Set pin's DRC info and possibly update pin->part's DRC info as well.
	// Then check for BOARDEDGE_PADHOLE, COPPERGRAPHIC_PADHOLE, RING_PAD, BOARDEDGE_PAD, and COPPERGRAPHIC_PAD errors
	CString d_str, x_str, y_str, str;
	drc_pin * drp = &pin->drc;
	CPart *part = pin->part;
	CDlgLog *log = m_dlg_log;
	int x = pin->x, y = pin->y;
	// CPT2.  Improved way of determining the diameter of dre circles that may need to be drawn:
	int dia = max(drp->max_x - drp->min_x, drp->max_y - drp->min_y);
	dia += 20*NM_PER_MIL;

	int hole = pin->ps->hole_size;
	if (hole)
	{
		// test clearance to board edge
		CIter<CBoard> ib (&boards);
		for (CBoard *b = ib.First(); b; b = ib.Next())
		{
			CIter<CSide> is (&b->main->sides);
			for (CSide *s = is.First(); s; s = is.Next())
			{
				int x1 = s->preCorner->x, y1 = s->preCorner->y;
				int x2 = s->postCorner->x, y2 = s->postCorner->y;
				// for now, only works for straight board edge segments
				if( s->m_style != CPolyline::STRAIGHT )
					continue;
				int d = ::GetClearanceBetweenLineSegmentAndPad( x1, y1, x2, y2, 0,
					PAD_ROUND, x, y, hole, 0, 0 );
				if( d < dr->board_edge_copper )
				{
					// BOARDEDGE_PADHOLE error
					::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
					CString s0 ((LPCSTR) IDS_PadHoleToBoardEdgeEquals);
					str.Format( s0, part->ref_des, pin->pin_name, d_str, x_str, y_str );
					CDre * dre = m_drelist->Add( CDre::BOARDEDGE_PADHOLE, &str,	pin, NULL, x, y, 0, 0, dia, 0 );
					if( dre && log )
						log->AddLine( "." );
				}
			}
		}
		// test clearance from graphic elements
		for( int igr=0; igr<part->m_outline_stroke.GetSize(); igr++ )
		{
			stroke * stk = &part->m_outline_stroke[igr];
			if (stk->layer < LAY_TOP_COPPER)
				continue;
			int d = ::GetClearanceBetweenLineSegmentAndPad( stk->xi, stk->yi, stk->xf, stk->yf, stk->w,
				PAD_ROUND, x, y, hole, 0, 0 );
			if( d <= dr->hole_copper )
			{
				// COPPERGRAPHIC_PADHOLE error
				::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
				::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
				::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
				CString s0 ((LPCSTR) IDS_PadHoleToCopperGraphics);
				str.Format( s0, part->ref_des, pin->pin_name, part->ref_des, pin->pin_name, d_str, x_str, y_str );
				CDre *dre = m_drelist->Add( CDre::COPPERGRAPHIC_PADHOLE, &str, pin, NULL, x, y, 0, 0, dia, 0 );
				if( dre && log)
					log->AddLine( "." );
			}
		}
	}

	// iterate through copper layers
	for( int layer = LAY_TOP_COPPER; layer < LAY_TOP_COPPER+m_num_copper_layers; layer++ )
	{
		// get test pad info
		int wid, len, r, type, hole, connect;
		BOOL bPad = pin->GetDrawInfo( layer, 0, 0, 0, 0,
			&type, &wid, &len, &r, &hole, &connect );
		if (!bPad) continue;

		/*** Check for errors involving the pad ***/
		if( type == PAD_NONE )
			continue;
		if( hole && pin->net )
		{
			// test annular ring
			int d = (wid - hole)/2;
			if( type == PAD_RECT || type == PAD_RRECT || type == PAD_OVAL )
				d = (min(wid,len) - hole)/2;
			if( d < dr->annular_ring_pins )
			{
				// RING_PAD
				::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
				::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
				::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
				CString s0 ((LPCSTR) IDS_AnnularRingEquals);
				str.Format( s0, part->ref_des, pin->pin_name, d_str, x_str, y_str );
				CDre * dre = m_drelist->Add( CDre::RING_PAD, &str, pin, NULL, x, y, 0, 0, dia, 0 );
				if( dre && log)
					log->AddLine( "." );
			}
		}
		// test clearance to board edge
		CIter<CBoard> ib (&boards);
		for (CBoard *b = ib.First(); b; b = ib.Next())
		{
			CIter<CCorner> ic (&b->main->corners);
			for (CCorner *c = ic.First(); c; c = ic.Next())
			{
				int x1 = c->x;
				int y1 = c->y;
				int x2 = c->postSide->postCorner->x;
				int y2 = c->postSide->postCorner->y;
				// for now, only works for straight board edge segments
				if( c->postSide->m_style != CPolyline::STRAIGHT )
					continue;
				int d = ::GetClearanceBetweenLineSegmentAndPad( x1, y1, x2, y2, 0,
					type, x, y, wid, len, r );
				if( d < dr->board_edge_copper )
				{
					// BOARDEDGE_PAD error
					::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
					CString s0 ((LPCSTR) IDS_PadToBoardEdgeEquals);
					str.Format( s0, part->ref_des, pin->pin_name, d_str, x_str, y_str );
					CDre * dre = m_drelist->Add( CDre::BOARDEDGE_PAD, &str, pin, NULL, x, y, 0, 0, dia, 0 );
					if( dre && log )
						log->AddLine( "." );
				}
			}
		}
		// test clearance from graphic elements
		for( int igr=0; igr<part->m_outline_stroke.GetSize(); igr++ )
		{
			stroke * stk = &part->m_outline_stroke[igr];
			if( stk->layer != layer )
				continue;
			int d = ::GetClearanceBetweenLineSegmentAndPad( stk->xi, stk->yi, stk->xf, stk->yf, stk->w,
				type, x, y, wid, len, r );
			if( d <= dr->copper_copper )
			{
				// COPPERGRAPHIC_PAD error
				::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
				::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
				::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
				CString s0 ((LPCSTR) IDS_PadToCopperGraphics);
				str.Format( s0, part->ref_des, pin->pin_name, part->ref_des, pin->pin_name, d_str, x_str, y_str );
				CDre * dre = m_drelist->Add( CDre::COPPERGRAPHIC_PAD, &str,
					pin, NULL, x, y, 0, 0, dia, 0 );
				if( dre && log )
						log->AddLine( "." );
			}
		}
	}
}

void CFreePcbDoc::DRCPinsAndCopperGraphics( CPart *part1, CPart *part2, int units, DesignRules *dr )
{
	// CPT2 new helper routine.  Compare the pins on part1 with the copper graphics for part2.  UNFINISHED.
	// First, iterate through copper graphic elements in part2 and get a mask of copper layers used.
	int graph_lay_mask = 0;
	for( int igr=0; igr<part2->m_outline_stroke.GetSize(); igr++ )
	{
		stroke * stk = &part2->m_outline_stroke[igr];
		if (stk->layer >= LAY_TOP_COPPER)
			graph_lay_mask |= 1<<stk->layer;
	}
	if (graph_lay_mask==0) 
		return;

	CIter<CPin> ipin1 (&part1->pins);
	for (CPin *pin1 = ipin1.First(); pin1; pin1 = ipin1.Next())
	{
		drc_pin *drp1 = &pin1->drc;
		// CPT2.  Improved way of determining the diameter of dre circles that may need to be drawn:
		int dia = max(drp1->max_x - drp1->min_x, drp1->max_y - drp1->min_y);
		dia += 20*NM_PER_MIL;

		// iterate through all copper layers
		for( int layer = LAY_TOP_COPPER; layer < LAY_TOP_COPPER+m_num_copper_layers; layer++ )
		{
			if (!((1<<layer) & graph_lay_mask ))
				continue;
			// there are graphics strokes on this layer.  Check the pin's pad
			int pad1_w, pad1_l, pad1_r, pad1_type, pad1_hole, pad1_connect;
			bool bPad1 = pin1->GetDrawInfo( layer, 0, 0, 0, 0,
				&pad1_type, &pad1_w, &pad1_l, &pad1_r, &pad1_hole, &pad1_connect );
			if( !bPad1 ) 
				continue;
			for( int igr=0; igr<part2->m_outline_stroke.GetSize(); igr++ )
			{
				stroke * stk = &part2->m_outline_stroke[igr];
				if( stk->layer != layer )
					continue;
				//** TODO: test for hole-copper violation
				//** TODO: test for pad-copper violation
			}
		}
	}
}

void CFreePcbDoc::DRCPinAndPin( CPin *pin1, CPin *pin2, int units, DesignRules *dr, int clearance )
{
	CString d_str, x_str, y_str, str;
	drc_pin *drp1 = &pin1->drc, *drp2 = &pin2->drc;
	CPart *part1 = pin1->part, *part2 = pin2->part;
	CDlgLog *log = m_dlg_log;

	if( drp1->hole_size && drp2->hole_size )
	{
		// test for hole-to-hole violation
		int dist = Distance( pin1->x, pin1->y, pin2->x, pin2->y );
		int h_h = max( 0, dist - (pin1->ps->hole_size + pin2->ps->hole_size)/2 );
		if( h_h < dr->hole_hole )
		{
			// PADHOLE_PADHOLE
			::MakeCStringFromDimension( &d_str, h_h, units, TRUE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &x_str, pin2->x, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, pin2->y, units, FALSE, TRUE, TRUE, 1 );
			CString s0 ((LPCSTR) IDS_PadHoleToPadeHoleEquals);
			str.Format( s0, part2->ref_des, pin2->pin_name, part1->ref_des, pin1->pin_name,
				d_str, x_str, y_str );
			CDre *dre = m_drelist->Add( CDre::PADHOLE_PADHOLE, &str, pin2, pin1, pin2->x, pin2->y, pin1->x, pin1->y, 0, 0 );
			if( dre && log )
				log->AddLine( "." );
		}
	}

	// see if pads on same layers
	if( !(drp1->layers & drp2->layers) )
		// no, see if either has a hole
		if( !drp1->hole_size && !drp2->hole_size )
			// no, go to next pin
			return;

	// see if padstacks might intersect
	if( drp1->min_x - drp2->max_x > clearance )
		return;
	if( drp2->min_x - drp1->max_x > clearance )
		return;
	if( drp1->min_y - drp2->max_y > clearance )
		return;
	if( drp2->min_y - drp1->max_y > clearance )
		return;

	// OK, pads might be too close
	// check for pad clearance violations on each layer
	for( int layer = LAY_TOP_COPPER; layer < LAY_TOP_COPPER+m_num_copper_layers; layer++ )
	{
		int pad1_w, pad1_l, pad1_r, pad1_type, pad1_hole, pad1_connect;
		bool bPad1 = pin1->GetDrawInfo( layer, 0, 0, 0, 0,
			&pad1_type, &pad1_w, &pad1_l, &pad1_r, &pad1_hole, &pad1_connect );
		if( !bPad1 )
			continue;
		int pad2_w, pad2_l, pad2_r, pad2_type, pad2_hole, pad2_connect;
		bool bPad2 = pin2->GetDrawInfo( layer, 0, 0, 0, 0, 
			&pad2_type, &pad2_w, &pad2_l, &pad2_r, &pad2_hole, &pad2_connect );
		if( !bPad2 )
			continue;
		if( pad2_hole )
		{
			// test for pad-padhole violation
			int dist = GetClearanceBetweenPads( pad1_type, pin1->x, pin1->y, pad1_w, pad1_l, pad1_r, 
				PAD_ROUND, pin2->x, pin2->y, pad2_hole, 0, 0 );
			if( dist < dr->hole_copper )
			{
				// PAD_PADHOLE 
				::MakeCStringFromDimension( &d_str, dist, units, TRUE, TRUE, TRUE, 1 );
				::MakeCStringFromDimension( &x_str, pin1->x, units, FALSE, TRUE, TRUE, 1 );
				::MakeCStringFromDimension( &y_str, pin1->y, units, FALSE, TRUE, TRUE, 1 );
				CString s0 ((LPCSTR) IDS_PadHoleToPadEquals);
				str.Format( s0, part2->ref_des, pin2->pin_name,	part1->ref_des, pin1->pin_name,	d_str, x_str, y_str );
				CDre * dre = m_drelist->Add( CDre::PAD_PADHOLE, &str, pin2, pin1, 
					pin2->x, pin2->y, pin1->x, pin1->y, 0, layer );
				if( dre && log )
					log->AddLine( "." );
				return;							// skip any more layers, go to next pin
			}
		}
		// test for pad-pad violation
		int dist = GetClearanceBetweenPads( pad1_type, pin1->x, pin1->y, pad1_w, pad1_l, pad1_r,
			pad2_type, pin2->x, pin2->y, pad2_w, pad2_l, pad2_r );
		if( dist < dr->pad_pad )
		{
			// PAD_PAD 
			::MakeCStringFromDimension( &d_str, dist, units, TRUE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &x_str, pin1->x, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, pin1->y, units, FALSE, TRUE, TRUE, 1 );
			CString s0 ((LPCSTR) IDS_PadToPadEquals);
			str.Format( s0, part2->ref_des, pin2->pin_name,	part1->ref_des, pin1->pin_name, d_str, x_str, y_str );
			CDre * dre = m_drelist->Add( CDre::PAD_PAD, &str, pin2, pin1, 
				pin2->x, pin2->y, pin1->x, pin1->y, 0, layer );
			if( dre && log )
				log->AddLine( "." );
			return;								// skip any more layers, go to next pin
		}
	}
}

void CFreePcbDoc::DRCArea(CArea *a, int units, DesignRules *dr)
{
	// CPT2 new helper for DRC.  Check a, and look for BOARDEDGE_COPPERAREA violations.  Also incorporates the old CPT routine
	// CheckBrokenArea().
	// CPT2 TODO wouldn't it suffice to check the main contour only?  Of course (either way) this method of checking could fail if the area
	// completely contains the whole board --- but do we really care?
	CString d_str, x_str, y_str, str;
	CDlgLog *log = m_dlg_log;
	int layer = a->m_layer;
	CNet *net = a->m_net;

	CIter<CContour> ictr (&a->contours);
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CIter<CSide> is (&ctr->sides);
		for (CSide *s = is.First(); s; s = is.Next())
		{
			int x1 = s->preCorner->x, y1 = s->preCorner->y;
			int x2 = s->postCorner->x, y2 = s->postCorner->y;
			// iterate through board outlines
			CIter<CBoard> ib (&boards);
			for (CBoard *b = ib.First(); b; b = ib.Next())
			{
				CIter<CSide> ibs (&b->main->sides);
				for (CSide *bs = ibs.First(); bs; bs = ibs.Next())
				{
					int bx1 = bs->preCorner->x, by1 = bs->postCorner->y;
					int bx2 = bs->postCorner->x, by2 = bs->postCorner->y;
					int x, y;
					int d = ::GetClearanceBetweenSegments( bx1, by1, bx2, by2, bs->m_style, 0,
						x1, y1, x2, y2, s->m_style, 0, dr->board_edge_copper, &x, &y );
					if( d < dr->board_edge_copper )
					{
						// BOARDEDGE_COPPERAREA error
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
						CString str0 ((LPCSTR) IDS_CopperAreaToBoardEdgeEquals);
						str.Format( str0, a->m_net->name, d_str, x_str, y_str );
						CDre * dre = m_drelist->Add( CDre::BOARDEDGE_COPPERAREA, &str, s, NULL, x, y, 0, 0, 0, 0 );
						if( dre && log )
							log->AddLine( "." );
					}
				}
			}
		}
	}

	// CPT: check for areas that have been broken or nearly broken by the clearances for traces from other nets (formerly routine CheckBrokenAreas()).
	// Create a "bitmap" (just a 2-d array of 1-byte flags, indicating whether a given location is part of the area's copper).
	// NB class CMyBitmap is found in utility.h.
	// Get area's bounds and dimensions in nm.
	CRect bounds = a->GetCornerBounds();
	int wNm = bounds.right - bounds.left, hNm = bounds.top - bounds.bottom;
	// Determine the granularity of the bitmap.  By default it's 10 mils per pixel, but that value may increase if area is big
	int gran = 10*NM_PER_MIL;	
	int w = wNm/gran, h = hNm/gran;
	while (w*h>1000000)
		w /= 2, h /= 2, gran *= 2;
	w += 3, h += 3;												// A little extra wiggle room...
	int x0 = bounds.left - gran, y0 = bounds.bottom - gran;
	int xSample = 0, ySample = 0;								// Set below:  an arbitrarily chosen edge-corner position
	CMyBitmap *bmp  = new CMyBitmap(w, h);

	// Draw the area's edges onto bmp, filling the relevant pixels in with byte value xEdge for outer edges
	// (edges belonging to a's contour #0) or xHole for edges of the area's holes (contours 1, 2, ...)
	#define xEdge 0x01
	#define xHole 0x02
	for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next()) 
	{
		CIter<CSide> is (&ctr->sides);
		for (CSide *s = is.First(); s; s = is.Next())
		{
			int x1 = s->preCorner->x, y1 = s->preCorner->y;
			if (!xSample)
				xSample = x1, ySample = y1;
			int x2 = s->postCorner->x, y2 = s->postCorner->y;
			x1 = (x1-x0)/gran, y1 = (y1-y0)/gran;
			x2 = (x2-x0)/gran, y2 = (y2-y0)/gran;
			char val = ctr==a->main? xEdge: xHole;
			int style = s->m_style;
			if (style==CPolyline::STRAIGHT)
				bmp->Line(x1,y1, x2,y2, val);
			else
				bmp->Arc(x1,y1, x2,y2, style==CPolyline::ARC_CW, val);
		}
	}

	// Now draw on the bitmap any segments that are in the correct layer but belong to nets other than a's.
	#define xSeg 0x03
	CIter<CNet> in (&m_nlist->nets);
	for (CNet *net2 = in.First(); net2; net2 = in.Next())
	{
		if (net2 == net) 
			continue;
		CIter<CConnect> ic (&net2->connects);
		for (CConnect *c2 = ic.First(); c2; c2 = ic.Next())
		{
			CIter<CSeg> is (&c2->segs);
			for (CSeg *s = is.First(); s; s = is.Next())
			{
				if (s->m_layer!=layer) 
					continue;
				int x1 = s->preVtx->x, y1 = s->preVtx->y;
				int x2 = s->postVtx->x, y2 = s->postVtx->y;
				x1 = (x1-x0)/gran, y1 = (y1-y0)/gran;
				x2 = (x2-x0)/gran, y2 = (y2-y0)/gran;
				bmp->Line(x1,y1, x2,y2, xSeg);
			}
		}
	}

	// Now scan the bitmap's rows, looking for one where the initial pixel values are 0/xSeg, 0/xSeg, ..., 0/xSeg, xEdge, 0.
	// The last pixel there is surely in the area's interior.  If we don't find such a pixel, the area is weirdly scrawny:  generate 
	// a DRC warning and bail out.
	int xInt = -1, yInt = -1;
	for (int y=1; y<h-1; y++) 
	{
		int x=0;
		while (x<w && (bmp->Get(x,y)==0 || bmp->Get(x,y)==xSeg)) x++;
		if (x>=w-2) 
			continue;
		if (bmp->Get(x,y)==xEdge && bmp->Get(x+1,y)==0) 
			{ xInt = x+1, yInt = y; break; }
	}
	if (xInt==-1) 
	{
		CString s ((LPCSTR) IDS_WarningCopperAreaIsTooNarrow), str;
		::MakeCStringFromDimension( &x_str, xSample, units, FALSE, TRUE, TRUE, 1 );
		::MakeCStringFromDimension( &y_str, ySample, units, FALSE, TRUE, TRUE, 1 );
		str.Format( s, net->name, x_str, y_str );
		CDre * dre = m_drelist->Add( CDre::COPPERAREA_BROKEN, &str, a, NULL, xSample, ySample, xSample, ySample, 0, 0 );
		if (dre && log) 
			log->AddLine( "." );
		delete(bmp);
		return;
	}

	// Flood-fill the area's interior, starting from the just-found point (xInt,yInt)
	#define xInterior 0x04
	bmp->FloodFill(xInt,yInt, xInterior);

	// Now scan the bitmap for any xEdge/xHole pixels that are not adjacent to an xInterior, and convert them to value xBad.
	//  If the area is nicely connected, then only a few odd corner pixels will be xBad
	#define xBad 0x05
	for (int y=1; y<h-1; y++) 
		for (int x=1; x<w-1; x++) 
		{
			if (bmp->Get(x,y) != xEdge && bmp->Get(x,y) != xHole) continue;
			if (bmp->Get(x-1,y) != xInterior && bmp->Get(x+1,y) != xInterior &&
				bmp->Get(x,y-1) != xInterior && bmp->Get(x,y+1) != xInterior) 
					bmp->Set(x,y, xBad);
		}

	// Now determine the connected components among the xBad pixels.  Each component that contains more than 2 (?) pixels results in
	// its own DRE.
	#define xBad2 0x06
	for (int y=1; y<h-1; y++) 
		for (int x=1; x<w-1; x++) 
		{
			if (bmp->Get(x,y) != xBad) continue;
			int nPix = bmp->FloodFill(x, y, xBad2);
			if (nPix<3) continue;
			int x2 = x*gran + x0, y2 = y*gran + y0;
			CString s ((LPCSTR) IDS_WarningCopperAreaIsTooNarrow), str;
			::MakeCStringFromDimension( &x_str, x2, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, y2, units, FALSE, TRUE, TRUE, 1 );
			str.Format( s, net->name, x_str, y_str );
			CDre * dre = m_drelist->Add( CDre::COPPERAREA_BROKEN, &str, a, NULL, x2, y2, x2, y2, 0, 0 );
			if ( dre && log )
				log->AddLine( "." );
		}
	
	// Done!
	delete(bmp);
}


void CFreePcbDoc::DRCConnect(CConnect *c, int units, DesignRules *dr)
{
	CString d_str, x_str, y_str, str;
	CNet *net = c->m_net;
	CDlgLog *log = m_dlg_log;
	int num_layers = LAY_TOP_COPPER + m_num_copper_layers;

	// get DRC info for this connection
	c->min_x = INT_MAX;		// bounding box for connection
	c->max_x = INT_MIN;
	c->min_y = INT_MAX;
	c->max_y = INT_MIN;
	c->vias_present = FALSE;
	c->seg_layers = 0;
	int max_trace_w = 0;	// maximum trace width for connection

	/*** iterate through connect's segs ***/
	CIter<CSeg> is (&c->segs);
	for (CSeg *s = is.First(); s; s = is.Next())
	{
		int x1 = s->preVtx->x, y1 = s->preVtx->y;
		int x2 = s->postVtx->x, y2 = s->postVtx->y;
		int w = s->m_width;
		int layer = s->m_layer;
		if (layer == LAY_RAT_LINE)
			// CPT2:  Neither TRACE_WIDTH nor BOARDEDGE_SEG matter for ratlines:
			continue;
		if( layer >= LAY_TOP_COPPER )
			c->seg_layers |= 1<<layer;
		// add segment to bounding box
		int seg_min_x = min( x1, x2 );
		int seg_min_y = min( y1, y2 );
		int seg_max_x = max( x1, x2 );
		int seg_max_y = max( y1, y2 );
		c->min_x = min( c->min_x, seg_min_x - w/2 );
		c->max_x = max( c->max_x, seg_max_x + w/2 );
		c->min_y = min( c->min_y, seg_min_y - w/2 );
		c->max_y = max( c->max_y, seg_max_y + w/2 );
		// test trace width
		if (w==0)
			continue;											// CPT2: the old code effectively did this.  Desirable?
		if( w < dr->trace_width )		
		{
			// TRACE_WIDTH error
			int x = (x1+x2)/2, y = (y1+y2)/2;
			::MakeCStringFromDimension( &d_str, w, units, TRUE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
			CString str0 ((LPCSTR) IDS_TraceWidthEquals);
			str.Format( str0, net->name, d_str, x_str, y_str );
			CDre * dre = m_drelist->Add( CDre::TRACE_WIDTH, &str, s, NULL, x, y, 0, 0, 0, layer );
			if( dre && log )
				log->AddLine( "." );
		}
		// test clearance to board edge
		CIter<CBoard> ib (&boards);
		for (CBoard *b = ib.First(); b; b = ib.Next())
		{
			CIter<CSide> ibs (&b->main->sides);
			for (CSide *bs = ibs.First(); bs; bs = ibs.Next())
			{
				int bx1 = bs->preCorner->x, by1 = bs->preCorner->y;
				int bx2 = bs->postCorner->x, by2 = bs->postCorner->y;
				int x, y;
				int d = ::GetClearanceBetweenSegments( bx1, by1, bx2, by2, bs->m_style, 0,
					x1, y1, x2, y2, CPolyline::STRAIGHT, w, dr->board_edge_copper, &x, &y );
				if( d < dr->board_edge_copper )
				{
					// BOARDEDGE_SEG error
					::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
					CString str0 ((LPCSTR) IDS_TraceToBoardEdgeEquals);
					str.Format( str0, net->name, d_str, x_str, y_str );
					CDre * dre = m_drelist->Add( CDre::BOARDEDGE_SEG, &str, s, NULL, x, y, 0, 0, 0, layer );
					if( dre && log )
						log->AddLine( "." );
				}
			}
		}
	}

	/*** iterate through connect's vertices (vias in particular) ***/
	CIter<CVertex> iv (&c->vtxs);
	for (CVertex *vtx = iv.First(); vtx; vtx = iv.Next())
	{
		if (!vtx->via_w) 
			continue;
		// via present.  CPT2 old code had a complex way of checking via-widths on each layer, but the results were never different from the
		// following.  (Are there plans one day for vias with custom widths on _each layer_?)
		int hole_w = vtx->tee? vtx->tee->via_hole_w: vtx->via_hole_w;
		int via_w = vtx->tee? vtx->tee->via_w: vtx->via_w;
		int dia = vtx->via_w + 20*NM_PER_MIL;					// Diameter for displayed dre circles, should any be generated
		c->min_x = min( c->min_x, vtx->x - via_w/2 );
		c->max_x = max( c->max_x, vtx->x + via_w/2 );
		c->min_y = min( c->min_y, vtx->y - via_w/2 );
		c->max_y = max( c->max_y, vtx->y + via_w/2 );
		int d = (via_w - hole_w)/2;
		if( d < dr->annular_ring_vias )
		{
			// RING_VIA
			::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &x_str, vtx->x, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, vtx->y, units, FALSE, TRUE, TRUE, 1 );
			CString s ((LPCSTR) IDS_ViaAnnularRingEquals);
			str.Format( s, net->name, d_str, x_str, y_str );
			CDre * dre = m_drelist->Add( CDre::RING_VIA, &str, vtx, NULL, vtx->x, vtx->y, 0, 0, dia, 0 );
			if( dre && log )
				log->AddLine( "." );
		}
		// test clearance to board edge
		CIter<CBoard> ib (&boards);
		for (CBoard *b = ib.First(); b; b = ib.Next())
		{
			CIter<CSide> ibs (&b->main->sides);
			for (CSide *bs = ibs.First(); bs; bs = ibs.Next())
			{
				int bx1 = bs->preCorner->x, by1 = bs->preCorner->y;
				int bx2 = bs->postCorner->x, by2 = bs->postCorner->y;
				//** for now, only works for straight board edge segments
				if (bs->m_style != CPolyline::STRAIGHT )
					continue;
				int d = ::GetClearanceBetweenLineSegmentAndPad( bx1, by1, bx2, by2, 0,
					PAD_ROUND, vtx->x, vtx->y, via_w, 0, 0 );
				if( d < dr->board_edge_copper )
				{
					// BOARDEDGE_VIA error
					::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &x_str, vtx->x, units, FALSE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &y_str, vtx->y, units, FALSE, TRUE, TRUE, 1 );
					CString s ((LPCSTR) IDS_ViaToBoardEdgeEquals);
					str.Format( s, net->name, d_str, x_str, y_str );
					CDre * dre = m_drelist->Add( CDre::BOARDEDGE_VIA, &str, vtx, NULL,
						vtx->x, vtx->y, 0, 0, dia, 0 );
					if( dre && log )
						log->AddLine( "." );
				}
				int dh = ::GetClearanceBetweenLineSegmentAndPad( bx1, by1, bx2, by2, 0,
					PAD_ROUND, vtx->x, vtx->y, hole_w, 0, 0 );
				if( dh < dr->board_edge_hole )
				{
					// BOARDEDGE_VIAHOLE error
					::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &x_str, vtx->x, units, FALSE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &y_str, vtx->y, units, FALSE, TRUE, TRUE, 1 );
					CString s ((LPCSTR) IDS_ViaHoleToBoardEdgeEquals);
					str.Format( s, net->name, d_str, x_str, y_str );
					CDre * dre = m_drelist->Add( CDre::BOARDEDGE_VIAHOLE, &str, vtx, NULL, 
						vtx->x, vtx->y, 0, 0, dia, 0 );
					if( dre && log )
						log->AddLine( "." );
				}
			}
		}
	}

	/*** iterate through all parts, checking for
			SEG_PAD, VIA_PAD, SEG_COPPERGRAPHIC, VIA_COPPERGRAPHIC errors  */
	CIter<CPart> ip (&m_plist->parts);
	for (CPart *part = ip.First(); part; part = ip.Next())
	{
		if (!part->shape) 
			continue;										// CPT2 this wasn't there before, but surely it makes sense?
		// if not on same layers, can't conflict
		if( !part->hole_flag && !c->vias_present && !(part->layers & c->seg_layers) )
			continue;

		// if bounding boxes don't overlap, can't conflict
		if( part->min_x - c->max_x > dr->pad_trace )
			continue;
		if( c->min_x - part->max_x > dr->pad_trace )
			continue;
		if( part->min_y - c->max_y > dr->pad_trace )
			continue;
		if( c->min_y - part->max_y > dr->pad_trace )
			continue;

		// OK, now we have to test each pad
		CIter<CPin> ipin (&part->pins);
		for (CPin *pin = ipin.First(); pin; pin = ipin.Next())
		{
			drc_pin * drp = &pin->drc;
			// if pin and connection bounds are separated enough, skip pin
			if( drp->min_x - c->max_x > dr->pad_trace )
				continue;
			if( c->min_x - drp->max_x > dr->pad_trace )
				continue;
			if( drp->min_y - c->max_y > dr->pad_trace )
				continue;
			if( c->min_y - drp->max_y > dr->pad_trace )
				continue;
			if( net == pin->net )
				// This condition existed in the code before, somewhat obscured.  But [CPT2 TODO] 
				// is it really tolerable to have a seg pass through a pin-hole from the same net?
				continue;
			// CPT2.  Improved way of determining the diameter of dre circles that may need to be drawn:
			int dia = max(drp->max_x - drp->min_x, drp->max_y - drp->min_y);
			dia += 20*NM_PER_MIL;

			// possible clearance violation, now test pad against each segment and via on each layer
			bool bPad[MAX_LAYERS];
			int pad_w[MAX_LAYERS], pad_l[MAX_LAYERS], pad_r[MAX_LAYERS];
			int pad_type[MAX_LAYERS], pad_hole[MAX_LAYERS], pad_connect[MAX_LAYERS];
			for (int i = LAY_TOP_COPPER; i < num_layers; i++)
				bPad[i] = pin->GetDrawInfo( i, 0, 0, 0, 0, &pad_type[i], &pad_w[i], &pad_l[i], &pad_r[i],
											&pad_hole[i], &pad_connect[i] );

			// CPT2: old code did a single loop through segs, and checked each seg's postVtx for via violations.  But this had the
			// effect that the head of a connect wouldn't get checked.  Therefore I've split it into 2 loops.
			CIter<CSeg> is (&c->segs);
			for (CSeg *s = is.First(); s; s = is.Next())
			{
				int layer = s->m_layer;
				if (layer < LAY_TOP_COPPER)						// e.g., a ratline
					continue;
				int w = s->m_width;
				int xi = s->preVtx->x, yi = s->preVtx->y;
				int xf = s->postVtx->x, yf = s->postVtx->y;
				int min_x = min( xi, xf ) - w/2;
				int max_x = max( xi, xf ) + w/2;
				int min_y = min( yi, yf ) - w/2;
				int max_y = max( yi, yf ) + w/2;

				if( drp->hole_size )
				{
					int d = GetClearanceBetweenLineSegmentAndPad( xi, yi, xf, yf, w,
							PAD_ROUND, pin->x, pin->y, pad_hole[layer], 0, 0 );
					if (d < dr->hole_copper ) 
					{
						// SEG_PADHOLE
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &x_str, pin->x, units, FALSE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &y_str, pin->y, units, FALSE, TRUE, TRUE, 1 );
						CString str0 ((LPCSTR) IDS_TraceToPadHoleEquals);
						str.Format( str0, net->name, part->ref_des, pin->pin_name,
							d_str, x_str, y_str );
						CDre * dre = m_drelist->Add( CDre::SEG_PAD, &str, s, pin, pin->x, pin->y, pin->x, pin->y, 
							dia, layer );
						if( dre && log )
							log->AddLine( "." );
					}
				}

				if( ((1<<layer) & drp->layers) && bPad[layer] && pad_type[layer] != PAD_NONE )
				{
					// pin has a pad on seg's layer (and boy did we check thoroughly)
					// check segment to pad clearance
					int d = GetClearanceBetweenLineSegmentAndPad( xi, yi, xf, yf, w,
						pad_type[layer], pin->x, pin->y, pad_w[layer], pad_l[layer], pad_r[layer] );
					if( d < dr->pad_trace ) 
					{
						// SEG_PAD
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &x_str, pin->x, units, FALSE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &y_str, pin->y, units, FALSE, TRUE, TRUE, 1 );
						CString str0 ((LPCSTR) IDS_TraceToPadEquals);
						str.Format( str0, net->name, part->ref_des, pin->pin_name,
							d_str, x_str, y_str );
						CDre * dre = m_drelist->Add( CDre::SEG_PAD, &str, s, pin, pin->x, pin->y, pin->x, pin->y, 
							dia, layer );
						if( dre && log )
							log->AddLine( "." );
					}
				}
			}		
					
			CIter<CVertex> iv (&c->vtxs);
			for (CVertex* v = iv.First(); v; v= iv.Next())
			{
				if (!v->via_w) continue;
				for (int layer = LAY_TOP_COPPER; layer < num_layers; layer++)
				{
					// compare via and pin on each of the various layers
					int test, via_w, via_hole_w;
					v->GetViaPadInfo( layer, &via_w, &via_hole_w, &test );
					if (!via_w)
						continue;
					if( bPad[layer] && pad_type[layer] != PAD_NONE )
					{
						// pin has a pad on this layer.  check via_pad to pin_pad clearance
						int d = GetClearanceBetweenPads( PAD_ROUND, v->x, v->y, via_w, 0, 0,
							pad_type[layer], pin->x, pin->y, pad_w[layer], pad_l[layer], pad_r[layer] );
						if( d < dr->pad_trace )
						{
							// VIA_PAD
							::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
							::MakeCStringFromDimension( &x_str, pin->x, units, FALSE, TRUE, TRUE, 1 );
							::MakeCStringFromDimension( &y_str, pin->y, units, FALSE, TRUE, TRUE, 1 );
							CString str0 ((LPCSTR) IDS_ViaPadToPadEquals);
							str.Format( str0, net->name, part->ref_des, pin->pin_name, d_str, x_str, y_str );
							CDre * dre = m_drelist->Add( CDre::VIA_PAD, &str, v, pin, v->x, v->y, pin->x, pin->y, 0, layer );
							if( dre && log )
								log->AddLine( "." );
							break;  // skip more layers
						}
						// also check via-hole to pin-pad clearance (CPT2 moved this here from further below)
						d = GetClearanceBetweenPads( PAD_ROUND, v->x, v->y, via_hole_w, 0, 0,
							pad_type[layer], pin->x, pin->y, pad_w[layer], pad_l[layer], pad_r[layer] );
						if( d < dr->hole_copper )
						{
							// VIAHOLE_PAD
							::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
							::MakeCStringFromDimension( &x_str, pin->x, units, FALSE, TRUE, TRUE, 1 );
							::MakeCStringFromDimension( &y_str, pin->y, units, FALSE, TRUE, TRUE, 1 );
							CString s ((LPCSTR) IDS_ViaHoleToPadEquals);
							str.Format( s, net->name, part->ref_des, pin->pin_name,
								d_str, x_str, y_str );
							CDre * dre = m_drelist->Add( CDre::VIA_PAD, &str, v, pin, v->x, v->y, pin->x, pin->y, 0, layer );
							if( dre && log )
								log->AddLine( "." );
							break;  // skip more layers
						}
					}
					if( drp->hole_size )
					{
						// pin has a hole, check via_pad to pin_hole clearance
						int d = Distance( v->x, v->y, pin->x, pin->y );
						d = max( 0, d - drp->hole_size/2 - via_w/2 );
						if( d < dr->hole_copper )
						{
							// VIA_PADHOLE
							::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
							::MakeCStringFromDimension( &x_str, pin->x, units, FALSE, TRUE, TRUE, 1 );
							::MakeCStringFromDimension( &y_str, pin->y, units, FALSE, TRUE, TRUE, 1 );
							CString s ((LPCSTR) IDS_ViaPadToPadHoleEquals);
							str.Format( s, net->name, part->ref_des, pin->pin_name,	d_str, x_str, y_str );
							CDre * dre = m_drelist->Add( CDre::VIA_PAD, &str, v, pin, v->x, v->y, pin->x, pin->y, 0, layer );
							if( dre && log )
								log->AddLine( "." );
							break;  // skip more layers
						}
					}
				}
				if( drp->hole_size )
				{
					// final comparison between pin and via:  hole vs. hole
					int d = Distance( v->x, v->y, pin->x, pin->y );
					d = max( 0, d - drp->hole_size/2 - v->via_hole_w/2 );
					if( d < dr->hole_hole )
					{
						// VIAHOLE_PADHOLE
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &x_str, pin->x, units, FALSE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &y_str, pin->y, units, FALSE, TRUE, TRUE, 1 );
						CString s ((LPCSTR) IDS_ViaHoleToPadHoleEquals);
						str.Format( s, net->name, part->ref_des, pin->pin_name, d_str, x_str, y_str );
						CDre * dre = m_drelist->Add( CDre::VIA_PAD, &str, v, pin, v->x, v->y, pin->x, pin->y, 0, LAY_TOP_COPPER );
						if( dre && log )
							log->AddLine( "." );
					}
				}
			}
		}

		// test each copper graphic stroke
		CArray<stroke> *ref_strokes = &part->m_ref->m_stroke;
		CArray<stroke> *value_strokes = &part->m_value->m_stroke;
		CArray<stroke> *outline_strokes = &part->m_outline_stroke;
		int n_ref = ref_strokes->GetSize(), n_value = value_strokes->GetSize(), n_outline = outline_strokes->GetSize();
		int n_total_strokes = n_ref + n_value + n_outline;
		for( int istk=0; istk<n_total_strokes; istk++ )
		{
			// get stroke to test
			stroke * test_stroke;
			if( istk < n_ref )
				test_stroke = &(*ref_strokes)[istk];
			else if( istk < n_ref + n_value )
				test_stroke = &(*value_strokes)[istk-n_ref];
			else 
				test_stroke = &(*outline_strokes)[istk-n_ref-n_value];
			int cglayer = test_stroke->layer;
			if (cglayer < LAY_TOP_COPPER) continue;
			int cgxi = test_stroke->xi;
			int cgyi = test_stroke->yi;
			int cgxf = test_stroke->xf;
			int cgyf = test_stroke->yf;
			int cgstyle;
			if( test_stroke->type == DL_LINE )
				cgstyle = CPolyline::STRAIGHT;
			else if( test_stroke->type == DL_ARC_CW )
				cgstyle = CPolyline::ARC_CW;
			else if( test_stroke->type == DL_ARC_CCW )
				cgstyle = CPolyline::ARC_CCW;
			else
				ASSERT(0);

			// test against each segment in connection
			CIter<CSeg> is (&c->segs);
			for (CSeg *s = is.First(); s; s = is.Next())
			{
				if (s->m_layer != test_stroke->layer)
					continue;
				int w = s->m_width;
				int xi = s->preVtx->x, yi = s->preVtx->y;
				int xf = s->postVtx->x, yf = s->postVtx->y;
				int min_x = min( xi, xf ) - w/2;
				int max_x = max( xi, xf ) + w/2;
				int min_y = min( yi, yf ) - w/2;
				int max_y = max( yi, yf ) + w/2;
				// check segment clearances
				int x, y;
				int d = ::GetClearanceBetweenSegments( cgxi, cgyi, cgxf, cgyf, cgstyle, 0,
					xi, yi, xf, yf, CPolyline::STRAIGHT, w,	dr->board_edge_copper, &x, &y );
				if( d < dr->copper_copper )
				{
					// COPPERGRAPHIC_SEG error
					::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
					CString s0 ((LPCSTR) IDS_TraceToCopperGraphic);
					str.Format( s0, net->name, d_str, x_str, y_str );
					CDre * dre = m_drelist->Add( CDre::COPPERGRAPHIC_SEG, &str, s, NULL, x, y, 0, 0, 0, cglayer );
					if( dre && log )
						log->AddLine( "." );
				}
			}
			// test against each via in connection
			CIter<CVertex> iv (&c->vtxs);
			for (CVertex *v = iv.First(); v; v = iv.Next())
			{
				if (!v->via_w) 
					continue;
				int test, via_w, via_hole_w;
				int dia = v->via_w + 20*NM_PER_MIL;									// Diameter for display dre's, if any get generated
				v->GetViaPadInfo( test_stroke->layer, &via_w, &via_hole_w, &test );
				if( via_w )
				{
					// check via
					int d = GetClearanceBetweenLineSegmentAndPad( cgxi, cgyi, cgxf, cgyf, test_stroke->w,
						PAD_ROUND, v->x, v->y, via_w, 0, 0 );
					if( d < dr->copper_copper )
					{
						// COPPERGRAPHIC_VIA error
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &x_str, v->x, units, FALSE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &y_str, v->y, units, FALSE, TRUE, TRUE, 1 );
						CString s0 ((LPCSTR) IDS_ViaToCopperGraphic);
						str.Format( s0,	net->name, d_str, x_str, y_str );
						CDre * dre = m_drelist->Add( CDre::COPPERGRAPHIC_VIA, &str, v, NULL, v->x, v->y, 
							0, 0, dia, cglayer );
						if( dre && log )
							log->AddLine( "." );
					}
				}
				if( via_hole_w )
				{
					// check via hole
					int d = GetClearanceBetweenLineSegmentAndPad( cgxi, cgyi, cgxf, cgyf, test_stroke->w,
						PAD_ROUND, v->x, v->y, via_hole_w, 0, 0 );
					if( d < dr->hole_copper )
					{
						// COPPERGRAPHIC_VIAHOLE error
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &x_str, v->x, units, FALSE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &y_str, v->y, units, FALSE, TRUE, TRUE, 1 );
						CString s0 ((LPCSTR) IDS_ViaHoleToCopperGraphic);
						str.Format( s0, net->name, d_str, x_str, y_str );
						CDre * dre = m_drelist->Add( CDre::COPPERGRAPHIC_VIAHOLE, &str, v, NULL, v->x, v->y,
							0, 0, dia, cglayer );
						if( dre && log )
							log->AddLine( "." );
					}
				}
			}
		}
	}
}

void CFreePcbDoc::DRCConnectAndConnect( CConnect *c1, CConnect *c2, int units, DesignRules *dr, int clearance )
{
	// CPT2 new helper for DRC().  Look for DRC violations involving c1 and c2 (which will belong to different nets)
	if( c1->min_x - c2->max_x > clearance )
		return;
	if( c1->min_y - c2->max_y > clearance )
		return;
	if( c2->min_x - c1->max_x > clearance )
		return;
	if( c2->min_y - c1->max_y > clearance )
		return;
	CString d_str, x_str, y_str, str;
	CDlgLog *log = m_dlg_log;
	CNet *net1 = c1->m_net, *net2 = c2->m_net;

	// now we have to compare all segments and vias in c1 against those in c2
	CIter<CSeg> is1 (&c1->segs);
	CIter<CSeg> is2 (&c2->segs);
	for (CSeg *s1 = is1.First(); s1; s1 = is1.Next())
	{
		// Get bounding rect for seg, including the vias on its endpoints
		CRect br1, br2;
		s1->GetBoundingRect( &br1 );
		int xi1 = s1->preVtx->x, yi1 = s1->preVtx->y;
		int xf1 = s1->postVtx->x, yf1 = s1->postVtx->y;
		for (CSeg *s2 = is2.First(); s2; s2 = is2.Next())
		{
			s2->GetBoundingRect( &br2 );
			int xi2 = s2->preVtx->x, yi2 = s2->preVtx->y;
			int xf2 = s2->postVtx->x, yf2 = s2->postVtx->y;
			// see if segment bounding rects are disjoint
			if( br1.left - br2.right > clearance )
				continue;
			if( br2.left - br1.right > clearance )
				continue;
			if( br1.bottom - br2.top > clearance )
				continue;
			if( br2.bottom - br1.top > clearance )
				continue;

			if( s1->m_layer == s2->m_layer && s1->m_layer >= LAY_TOP_COPPER ) 
			{
				// test clearances between segments on the same layer
				int xx, yy; 
				int d = ::GetClearanceBetweenSegments( xi1, yi1, xf1, yf1, CPolyline::STRAIGHT, s1->m_width, 
					xi2, yi2, xf2, yf2, CPolyline::STRAIGHT, s2->m_width, dr->trace_trace, &xx, &yy );
				if( d < dr->trace_trace )
				{
					// SEG_SEG
					::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &x_str, xx, units, FALSE, TRUE, TRUE, 1 );
					::MakeCStringFromDimension( &y_str, yy, units, FALSE, TRUE, TRUE, 1 );
					CString s0 ((LPCSTR) IDS_TraceToTraceEquals);
					str.Format( s0, net1->name, net2->name, d_str, x_str, y_str );
					CDre * dre = m_drelist->Add( CDre::SEG_SEG, &str, s1, s2, xx, yy, xx, yy, 0, s1->m_layer );
					if( dre && log )
						log->AddLine( "." );
				}
			}

			// test clearances between segments and vias.  In all cases look at the postVtx;  if either seg is first in the
			// connect, look at their preVtx's also
			DRCSegmentAndVia(s1, s2->postVtx, units, dr);
			DRCSegmentAndVia(s2, s1->postVtx, units, dr);
			if (!s1->preVtx->preSeg)
				DRCSegmentAndVia(s2, s1->preVtx, units, dr);
			if (!s2->preVtx->preSeg)
				DRCSegmentAndVia(s1, s2->preVtx, units, dr);
			// test clearances between postVtx vias.  In case either seg is first in the connect, compare preVtx vias also
			DRCViaAndVia(s1->postVtx, s2->postVtx, units, dr);
			if (!s1->preVtx->preSeg)
				DRCViaAndVia(s1->preVtx, s2->postVtx, units, dr);
			if (!s2->preVtx->preSeg)
				DRCViaAndVia(s1->postVtx, s2->preVtx, units, dr);
			if (!s1->preVtx->preSeg && !s2->preVtx->preSeg)
				DRCViaAndVia(s1->preVtx, s2->preVtx, units, dr);
		}
	}
}



void CFreePcbDoc::DRCSegmentAndVia(CSeg *seg, CVertex *vtx, int units, DesignRules *dr)
{
	// CPT2 new helper routine for DRC().  Compare seg and vtx (checking first that seg is not a ratline and vtx is a via), and generate
	// any d-r errors that may come up.
	CString d_str, x_str, y_str, str;
	CDlgLog *log = m_dlg_log;
	int layer = seg->m_layer;
	if (layer < LAY_TOP_COPPER)
		return;															// e.g., a ratline
	if (!vtx->via_w)
		return;
	int xi = seg->preVtx->x, yi = seg->preVtx->y;
	int xf = seg->postVtx->x, yf = seg->postVtx->y;
	int test = vtx->GetViaConnectionStatus( layer );
	int via_w  = vtx->via_w;											// normal via pad
	int dia = via_w + 20*NM_PER_MIL;									// Diameter for displayed dre circles, should any be generated
	if( layer > LAY_BOTTOM_COPPER && test == CVertex::VIA_NO_CONNECT )
		// inner layer and no trace or thermal, so no via pad
		via_w = 0;
	else if( layer > LAY_BOTTOM_COPPER && (test & CVertex::VIA_AREA) && !(test & CVertex::VIA_TRACE) )
		// inner layer with small thermal, use annular ring
		via_w = vtx->via_hole_w + 2*dr->annular_ring_vias;

	// check clearance
	if( via_w )
	{
		// check clearance between segment and via pad
		int d = GetClearanceBetweenLineSegmentAndPad( xi, yi, xf, yf, seg->m_width,
			PAD_ROUND, vtx->x, vtx->y, vtx->via_w, 0, 0 );
		if( d < dr->trace_trace )
		{
			// SEG_VIA
			::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &x_str, vtx->x, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, vtx->y, units, FALSE, TRUE, TRUE, 1 );
			CString s0 ((LPCSTR) IDS_TraceToViaPadEquals);
			str.Format( s0, seg->m_net->name, vtx->m_net->name,	d_str, x_str, y_str );
			CDre * dre = m_drelist->Add( CDre::SEG_VIA, &str, seg, vtx, vtx->x, vtx->y, vtx->x, vtx->y, dia, seg->m_layer );
			if( dre && log )
				log->AddLine( "." );
		}
	}

	// check clearance between segment and via hole
	int d = GetClearanceBetweenLineSegmentAndPad( xi, yi, xf, yf, seg->m_width,
		PAD_ROUND, vtx->x, vtx->y, vtx->via_hole_w, 0, 0 );
	if( d < dr->hole_copper )
	{
		// SEG_VIAHOLE
		::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
		::MakeCStringFromDimension( &x_str, vtx->x, units, FALSE, TRUE, TRUE, 1 );
		::MakeCStringFromDimension( &y_str, vtx->y, units, FALSE, TRUE, TRUE, 1 );
		CString s0 ((LPCSTR) IDS_TraceToViaHoleEquals);
		str.Format( s0, seg->m_net->name, vtx->m_net->name,	d_str, x_str, y_str );
		CDre * dre = m_drelist->Add( CDre::SEG_VIAHOLE, &str, seg, vtx, vtx->x, vtx->y, vtx->x, vtx->y, dia, seg->m_layer );
		if( dre && log )
			log->AddLine( "." );
	}
}

void CFreePcbDoc::DRCViaAndVia(CVertex *vtx1, CVertex *vtx2, int units, DesignRules *dr)
{
	if( !vtx1->via_w || !vtx2->via_w )
		return;
	CString d_str, x_str, y_str, str;
	CDlgLog *log = m_dlg_log;

	for( int layer = LAY_TOP_COPPER; layer < LAY_TOP_COPPER+m_num_copper_layers; layer++ )
	{
		// get size of vtx1's via pad on this layer
		int test = vtx1->GetViaConnectionStatus( layer );
		int via_w1 = vtx1->via_w;	// normal via pad
		if( layer > LAY_BOTTOM_COPPER && test == CVertex::VIA_NO_CONNECT )
			// inner layer and no trace or thermal, so no via pad
			via_w1 = 0;
		else if( layer > LAY_BOTTOM_COPPER && (test & CVertex::VIA_AREA) && !(test & CVertex::VIA_TRACE) )
			// inner layer with small thermal, use annular ring
			via_w1 = vtx1->via_hole_w + 2*dr->annular_ring_vias;
		// get size of vtx2's via pad on this layer
		test = vtx2->GetViaConnectionStatus( layer );
		int via_w2 = vtx2->via_w;	// normal via pad
		if( layer > LAY_BOTTOM_COPPER && test == CVertex::VIA_NO_CONNECT )
			// inner layer and no trace or thermal, so no via pad
			via_w2 = 0;
		else if( layer > LAY_BOTTOM_COPPER && (test & CVertex::VIA_AREA) && !(test & CVertex::VIA_TRACE) )
			// inner layer with small thermal, use annular ring
			via_w2 = vtx2->via_hole_w + 2*dr->annular_ring_vias;
		if( !via_w1 || !via_w2 )
			continue;

		// check clearance between pads
		int d = GetClearanceBetweenPads( PAD_ROUND, vtx1->x, vtx1->y, vtx1->via_w, 0, 0, 
			PAD_ROUND, vtx2->x, vtx2->y, vtx2->via_w, 0, 0 );
		if( d < dr->trace_trace )
		{
			// VIA_VIA
			::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &x_str, vtx1->x, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, vtx1->y, units, FALSE, TRUE, TRUE, 1 );
			CString s ((LPCSTR) IDS_ViaPadToViaPadEquals);
			str.Format( s, vtx1->m_net->name, vtx2->m_net->name, d_str, x_str, y_str );
			CDre * dre = m_drelist->Add( CDre::VIA_VIA, &str, vtx1, vtx2, vtx1->x, vtx1->y, vtx2->x, vtx2->y, 0, layer );
			if( dre && log )
				log->AddLine( "." );
		}
		// check vtx1's pad versus vtx2's hole
		d = GetClearanceBetweenPads( PAD_ROUND, vtx1->x, vtx1->y, vtx1->via_w, 0, 0, 
			PAD_ROUND, vtx2->x, vtx2->y, vtx2->via_hole_w, 0, 0 );
		if( d < dr->hole_copper )
		{
			// VIA_VIAHOLE
			::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &x_str, vtx1->x, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, vtx1->y, units, FALSE, TRUE, TRUE, 1 );
			CString s ((LPCSTR) IDS_ViaPadToViaHoleEquals);
			str.Format( s, vtx1->m_net->name, vtx2->m_net->name, d_str, x_str, y_str );
			CDre * dre = m_drelist->Add( CDre::VIA_VIAHOLE, &str, vtx1, vtx2, vtx1->x, vtx1->y, vtx2->x, vtx2->y, 0, layer );
			if( dre && log )
				log->AddLine( "." );
		}
		// check vtx2's pad versus vtx1's hole
		d = GetClearanceBetweenPads( PAD_ROUND, vtx1->x, vtx1->y, vtx1->via_hole_w, 0, 0,
			PAD_ROUND, vtx2->x, vtx2->y, vtx2->via_w, 0, 0 );
		if( d < dr->hole_copper )
		{
			// VIA_VIAHOLE
			::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &x_str, vtx2->x, units, FALSE, TRUE, TRUE, 1 );
			::MakeCStringFromDimension( &y_str, vtx2->y, units, FALSE, TRUE, TRUE, 1 );
			CString s ((LPCSTR) IDS_ViaPadToViaHoleEquals);
			str.Format( s, vtx2->m_net->name, vtx1->m_net->name, d_str, x_str, y_str );
			CDre * dre = m_drelist->Add( CDre::VIA_VIAHOLE, &str, vtx2, vtx1, vtx2->x, vtx2->y, vtx1->x, vtx1->y, 0, layer );
			if( dre && log )
				log->AddLine( "." );
		}
	}

	// compare via holes for clearance
	int d = GetClearanceBetweenPads( PAD_ROUND, vtx1->x, vtx1->y, vtx1->via_hole_w, 0, 0, 
		PAD_ROUND, vtx2->x, vtx2->y, vtx2->via_hole_w, 0, 0  );
	if( d < dr->hole_hole )
	{
		// VIA_VIAHOLE
		::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
		::MakeCStringFromDimension( &x_str, vtx1->x, units, FALSE, TRUE, TRUE, 1 );
		::MakeCStringFromDimension( &y_str, vtx1->y, units, FALSE, TRUE, TRUE, 1 );
		CString s ((LPCSTR) IDS_ViaHoleToViaHoleEquals);
		str.Format( s, vtx1->m_net->name, vtx2->m_net->name, d_str, x_str, y_str );
		CDre * dre = m_drelist->Add( CDre::VIAHOLE_VIAHOLE, &str, vtx1, vtx2, vtx1->x, vtx1->y, vtx2->x, vtx2->y, 0, 0 );
		if( dre && log )
			log->AddLine( "." );
	}
}

void CFreePcbDoc::DRCAreaAndArea( CArea *a1, CArea *a2, int units, DesignRules *dr )
{
	if (a1->m_layer!= a2->m_layer)
		return;
	CString d_str, x_str, y_str, str;
	CDlgLog *log = m_dlg_log;
	CNet *net1 = a1->m_net, *net2 = a2->m_net;

	// Test spacing between areas.  Check all pairs of sides from all contours
	bool bErrorFound = false;
	CIter<CContour> ictr1 (&a1->contours);
	CIter<CContour> ictr2 (&a2->contours);
	for (CContour *ctr1 = ictr1.First(); ctr1; ctr1 = ictr1.Next())
		for (CContour *ctr2 = ictr2.First(); ctr2; ctr2 = ictr2.Next())
		{
			CIter<CSide> is1 (&ctr1->sides);
			CIter<CSide> is2 (&ctr2->sides);
			for (CSide *s1 = is1.First(); s1; s1 = is1.Next())
				for (CSide *s2 = is2.First(); s2; s2 = is2.Next())
				{
					int xi1 = s1->preCorner->x, yi1 = s1->preCorner->y;
					int xf1 = s1->postCorner->x, yf1 = s1->postCorner->y;
					int xi2 = s2->preCorner->x, yi2 = s2->preCorner->y;
					int xf2 = s2->postCorner->x, yf2 = s2->postCorner->y;
					int x, y;
					int d = ::GetClearanceBetweenSegments( xi1, yi1, xf1, yf1, s1->m_style, 0,
						xi2, yi2, xf2, yf2, s2->m_style, 0, dr->copper_copper, &x, &y );
					if( d < dr->copper_copper )
					{
						// COPPERAREA_COPPERAREA error
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &x_str, x, units, FALSE, TRUE, TRUE, 1 );
						::MakeCStringFromDimension( &y_str, y, units, FALSE, TRUE, TRUE, 1 );
						CString str0 ((LPCSTR) IDS_CopperAreaToCopperArea);
						str.Format( str0, net1->name, net2->name, d_str, x_str, y_str );
						CDre * dre = m_drelist->Add( CDre::COPPERAREA_COPPERAREA, &str, s1, s2, x, y, x, y, 0, 0 );
						if( dre && log )
							log->AddLine( "." );
						bErrorFound = true;
					}
				}
		}
					
	// For completeness, test if a1 has a main-contour point inside a2, or vice versa.  This covers cases where one area is totally
	// contained within another.  CPT2 optimization:  Don't bother if a clearance error for a1 and a2 was already found.
	if (bErrorFound) return;
	CIter<CCorner> ic1 (&a1->main->corners);
	for (CCorner *c1 = ic1.First(); c1; c1 = ic1.Next())
		if( a2->TestPointInside( c1->x, c1->y ) )
		{
			// COPPERAREA_COPPERAREA error
			CString s ((LPCSTR) IDS_CopperAreaInsideCopperArea);
			str.Format( s, net1->name, net2->name );
			CDre * dre = m_drelist->Add( CDre::COPPERAREA_INSIDE_COPPERAREA, &str, a1, a2, c1->x, c1->y, c1->x, c1->y, 0, 0 );
			if( dre && log )
				log->AddLine( "." );
			break;								// One corner error of this sort suffices.
		}
	CIter<CCorner> ic2 (&a2->main->corners);
	for (CCorner *c2 = ic2.First(); c2; c2 = ic2.Next())
		if( a1->TestPointInside( c2->x, c2->y ) )
		{
			// COPPERAREA_COPPERAREA error
			CString s ((LPCSTR) IDS_CopperAreaInsideCopperArea);
			str.Format( s, net2->name, net1->name );
			CDre * dre = m_drelist->Add( CDre::COPPERAREA_INSIDE_COPPERAREA, &str, a1, a2, c2->x, c2->y, c2->x, c2->y, 0, 0 );
			if( dre && log )
				log->AddLine( "." );
			break;								// One corner error of this sort suffices.
		}
}

void CFreePcbDoc::DRCUnrouted(int units) 
{
	// CPT2 new helper for DRC().  Check for any unrouted connections
	CDlgLog *log = m_dlg_log;
	CIter<CNet> in (&m_nlist->nets);
	for (CNet *net = in.First(); net; net = in.Next())
	{
		CIter<CConnect> ic (&net->connects);
		for (CConnect *c = ic.First(); c; c = ic.Next())
		{
			// check for unrouted or partially routed connection
			bool bUnrouted = false;
			CIter<CSeg> is (&c->segs);
			for (CSeg *s = is.First(); s; s = is.Next())
				if( s->m_layer == LAY_RAT_LINE )
					{ bUnrouted = TRUE;	break; }
			if( !bUnrouted )
				continue;
			// unrouted or partially routed connection
			CString from_str, to_str, str, str0;
			c->head->GetTypeStatusStr( &from_str );
			c->tail->GetTypeStatusStr( &to_str );
			if( c->NumSegs() > 1 )
				str0.LoadStringA( IDS_PartiallyRoutedConnection );
			else
				str0.LoadStringA( IDS_UnroutedConnection );
			str.Format( str0, net->name, from_str, to_str );
			// Draw the dre circle halfway along the first seg:
			CVertex *v1 = c->head, *v2 = v1->postSeg->postVtx;
			int x = (v1->x + v2->x) / 2;
			int y = (v1->y + v2->y) / 2;
			CDre * dre = m_drelist->Add( CDre::UNROUTED, &str, c, NULL, x, y, x, y, 0, 0 );
			if( dre && log )
				log->AddLine( "." );
		}
	}
}

void CFreePcbDoc::CheckDefaultCfg()
{
	HANDLE h = CreateFile(m_defaultcfg_dir + "\\default.cfg", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
	{
		CString str ((LPCSTR) IDS_WarningFileDefaultCfgIsStoredInAFolder);
		AfxMessageBox(str);
		OnToolsPreferences();
	}
	else
		CloseHandle(h);
}
