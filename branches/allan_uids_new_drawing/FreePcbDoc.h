// FreePcbDoc.h : interface of the CFreePcbDoc class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_FREEPCBDOC_H__A00395C2_2CF4_4902_9C7B_CBB16DB58836__INCLUDED_)
#define AFX_FREEPCBDOC_H__A00395C2_2CF4_4902_9C7B_CBB16DB58836__INCLUDED_

#include "stdafx.h"

#pragma once
#include "NetList.h"
#include "TextList.h"
#include "PcbFont.h"
#include "SMfontutil.h"
#include "SMcharacter.h"
#include "UndoBuffer.h"
#include "UndoList.h"
#include "FootprintLib.h"
#include "DlgDRC.h"
#include "DesignRules.h"
//#include "QAFDebug.h"

class CFreePcbDoc;
class CFreePcbView;



struct undo_board_outline {
	// undo_poly struct starts here 
};

struct undo_sm_cutout {
	int layer;
	int hatch_style;
	int ncorners;
	// array of undo_corners starts here
};

struct undo_move_origin {
	int x_off;
	int y_off;
};

class CFreePcbDoc : public CDocument
{
public:

protected: // create from serialization only
	CFreePcbDoc();
	DECLARE_DYNCREATE(CFreePcbDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFreePcbDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFreePcbDoc();
	void OnTimer();
	BOOL FileOpen( LPCTSTR fn, BOOL bLibrary=FALSE );
	int FileClose();
	void FileLoadLibrary( LPCTSTR pathname );
	void ProjectModified( BOOL flag, BOOL b_clear_redo=TRUE );
	void InitializeNewProject();
	void CFreePcbDoc::SendInitialUpdate();
	void ReadFootprints( CStdioFile * pcb_file, 
		CMapStringToPtr * cache_map=NULL,
		BOOL bFindSection=TRUE );
	int WriteFootprints( CStdioFile * file, CMapStringToPtr * cache_map=NULL );
	CShape * GetFootprintPtr( CString name );
	void MakeLibraryMaps( CString * fullpath );
	CPolyLine * GetBoardOutlineByUID( int uid, int * index=NULL );
	CPolyLine * GetMaskCutoutByUID( int uid, int * index=NULL );
	void ReadBoardOutline( CStdioFile * pcb_file, CArray<CPolyLine> * bd=NULL );
	void WriteBoardOutline( CStdioFile * pcb_file, CArray<CPolyLine> * bd=NULL );
	void ReadSolderMaskCutouts( CStdioFile * pcb_file, CArray<CPolyLine> * sm=NULL );
	void WriteSolderMaskCutouts( CStdioFile * pcb_file, CArray<CPolyLine> * sm=NULL );
	void ReadOptions( CStdioFile * pcb_file );
	void WriteOptions( CStdioFile * pcb_file );
	int ImportNetlist( CStdioFile * file, UINT flags, 
						partlist_info * pl, netlist_info * nl );
	int ImportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pl, netlist_info * nl );
	int ExportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pl, netlist_info * nl );
	void ImportSessionFile( CString * filepath, CDlgLog * log=NULL, BOOL bVerbose=TRUE );
	undo_move_origin * CreateMoveOriginUndoRecord( int x_off, int y_off );
	static void MoveOriginUndoCallback( int type, void * ptr, BOOL undo );
	undo_board_outline * CreateBoardOutlineUndoRecord( CPolyLine * poly );
	static void BoardOutlineUndoCallback( int type, void * ptr, BOOL undo );
	undo_sm_cutout * CreateSMCutoutUndoRecord( CPolyLine * poly );
	static void SMCutoutUndoCallback( int last_flag, void * ptr, BOOL undo );
	void OnFileAutoOpen( LPCTSTR fn );
	BOOL FileSave( CString * folder, CString * filename, 
		CString * old_folder, CString * old_filename,
		BOOL bBackup=TRUE );
	BOOL AutoSave();
	void SetFileLayerMap( int file_layer, int layer );
	void PurgeFootprintCache();
	void ResetUndoState();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

public:
	double m_version;			// version number, such as "1.105"
	double m_file_version;		// the oldest version of FreePCB that can read
								// files created with this version
	double m_read_version;		// the version from the project file
	BOOL bNoFilesOpened;		// TRUE if no files have been opened
	BOOL m_edit_footprint;		// TRUE to edit footprint of selected part
	BOOL m_project_open;		// FALSE if no project open
	BOOL m_project_modified;	// FALSE if project not modified since loaded
	BOOL m_project_modified_since_autosave;	// FALSE if project not modified since loaded
	BOOL m_footprint_modified;	// FALSE if the footprint being edited has not changed
	BOOL m_footprint_name_changed;	// TRUE if the footprint being edited has had its name changed
	CString m_window_title;		// window title for PCB editor
	CString m_fp_window_title;	// window title for footprint editor
	CString m_name;				// project name
	CString m_app_dir;			// application directory (full path) 
	CString m_lib_dir;			// path to default library folder (may be relative)
	CString m_full_lib_dir;		// full path to default library folder
	CString m_parent_folder;	// path to parent of project folders (may be relative)
	CString m_path_to_folder;	// path to project folder
	CString m_pcb_filename;		// name of project file
	CString m_pcb_full_path;	// full path to project file
	CString m_cam_full_path;	// full path to CAM file folder
	CString m_netlist_full_path;	// last netlist loaded
	CArray<CPolyLine> m_board_outline;	// PCB outline
	CDisplayList * m_dlist;		// display list
	CDisplayList * m_dlist_fp;	// display list for footprint editor
	CPartList * m_plist;		// part list
	SMFontUtil * m_smfontutil;	// Hershey font utility
	CNetList * m_nlist;			// net list
	CTextList * m_tlist;		// text list
	CMapStringToPtr m_footprint_cache_map;	// map of footprints cached in memory
	CFreePcbView * m_view;		// pointer to CFreePcbView 
	int m_file_close_ret;		// return value from OnFileClose() dialog
	CFootLibFolderMap m_footlibfoldermap;
	CDlgLog * m_dlg_log;
	DRErrorList * m_drelist;
	CArray<CPolyLine> m_sm_cutout;	// array of soldermask cutouts

	// undo and redo stacks and state
	BOOL m_bLastPopRedo;		// flag that last stack op was pop redo
	CUndoList * m_undo_list;	// undo stack
	CUndoList * m_redo_list;	// redo stack

	// autorouter file parameters
	int m_dsn_flags;			// options for DSN export
	BOOL m_dsn_bounds_poly;		
	BOOL m_dsn_signals_poly;
	CString m_ses_full_path;	// full path to last SES file

	// netlist import options
	int m_import_flags;

	// project options
	BOOL m_bSMT_copper_connect;
	int m_default_glue_w;
	BOOL m_auto_ratline_disable;
	int m_auto_ratline_min_pins;

	// pseudo-clipboard
	CPartList * clip_plist;
	CNetList * clip_nlist;
	CTextList * clip_tlist;
	CArray<CPolyLine> clip_sm_cutout;
	CArray<CPolyLine> clip_board_outline;

	// define world units for CDisplayList
	int m_pcbu_per_wu;

	// grids and units for pcb editor
	int m_units;					// MM or MIL
	double m_visual_grid_spacing;	// grid spacing
	double m_part_grid_spacing;		// grid spacing
	double m_routing_grid_spacing;	// grid spacing
	int m_snap_angle;				// 0, 45 or 90
	CArray<double> m_visible_grid;	// array of choices for visible grid
	CArray<double> m_part_grid;		// array of choices for placement grid
	CArray<double> m_routing_grid;	// array of choices for routing grid

	// CPT.  Allow for "hidden" grid values (which are seen as unchecked items in the grid-values dialogs)
	CArray<double> m_visible_grid_hidden;
	CArray<double> m_part_grid_hidden;	
	CArray<double> m_routing_grid_hidden;
	// end CPT

	// grids and units for footprint editor
	int m_fp_units;						// MM or MIL
	double m_fp_visual_grid_spacing;	// grid spacing
	double m_fp_part_grid_spacing;		// grid spacing
	int m_fp_snap_angle;				// 0, 45 or 90
	CArray<double> m_fp_visible_grid;	// array of choices for visible grid
	CArray<double> m_fp_part_grid;		// array of choices for placement grid

	// CPT.  Allow for "hidden" grid values (which are seen as unchecked items in the grid-values dialogs)
	CArray<double> m_fp_visible_grid_hidden;
	CArray<double> m_fp_part_grid_hidden;	
	// end CPT

	// layers
	int m_num_layers;			// number of drawing layers (note: different than copper layers)
	int m_num_copper_layers;	// number of copper layers
	int m_rgb[MAX_LAYERS][3];	// array of RGB values for each drawing layer
	int m_vis[MAX_LAYERS];		// array of visible flags

	// layers for footprint editor
	int m_fp_num_layers;
	int m_fp_rgb[MAX_LAYERS][3];
	int m_fp_vis[MAX_LAYERS];

	// default trace and via widths for routing
	int m_trace_w;			// default trace width
	int m_via_w;			// default via pad width
	int m_via_hole_w;		// default via hole diameter
	CArray<int> m_w;		// array of trace widths 
	CArray<int> m_v_w;		// array of via widths
	CArray<int> m_v_h_w;	// array of via hole widths

	// CAM flags and parameters
	int m_cam_flags;
	int m_cam_units;
	int m_fill_clearance; 
	int m_mask_clearance;
	int m_paste_shrink;
	int m_thermal_width;
	int m_min_silkscreen_stroke_wid;
	int m_pilot_diameter;
	int m_outline_width;
	int m_hole_clearance;
	int m_cam_layers;
	int m_cam_drill_file;
	int m_annular_ring_pins;
	int m_annular_ring_vias;
	int m_n_x, m_n_y, m_space_x, m_space_y;

	// report file options
	int m_report_flags;

	// autosave times
	int m_auto_interval;	// interval (sec)
	int m_auto_elapsed;		// time since last save (sec)

	// CPT:  new preference values
	bool m_bReversePgupPgdn;
	bool m_bLefthanded;
	// end CPT

	//DRC limits
	DesignRules m_dr;

	// Brian's stuff
	int m_thermal_clearance;


// Generated message map functions
public:
	//{{AFX_MSG(CFreePcbDoc)
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnAddPart();
	afx_msg void OnProjectNetlist();
	afx_msg void OnFileOpen();
	afx_msg void OnFileNew();
	afx_msg void OnFileClose();
	afx_msg void OnViewLayers();
	afx_msg void OnProjectPartlist();
	afx_msg void OnPartProperties();
	afx_msg void OnFileImport();
	afx_msg void OnAppExit();
	afx_msg void OnFileConvert();
	afx_msg void OnEditUndo();
	afx_msg void OnFileGenerateCadFiles();
	afx_msg void OnToolsFootprintwizard();
	afx_msg void OnProjectOptions();
	afx_msg void OnFileExport();
	afx_msg void OnToolsCheckPartsAndNets();
	afx_msg void OnToolsDrc();
	afx_msg void OnToolsClearDrc();
	afx_msg void OnToolsShowDRCErrorlist();
	afx_msg void OnToolsCheckConnectivity();
	afx_msg void OnViewLog();
	afx_msg void OnToolsCheckCopperAreas();
	afx_msg void OnToolsCheckTraces();
	afx_msg void OnEditPasteFromFile();
	afx_msg void OnFilePrint();
	afx_msg void OnFileExportDsn();
	afx_msg void OnFileImportSes();
	afx_msg void OnEditRedo();
	afx_msg void OnRepeatDrc();
	afx_msg void OnFileGenerateReportFile();
	afx_msg void OnProjectCombineNets();
	afx_msg void OnFileLoadLibrary();
	afx_msg void OnFileSaveLibrary();
	// CPT
	afx_msg void OnToolsPreferences();
	afx_msg void OnViewRoutingGrid();
	afx_msg void OnViewVisibleGrid();
	afx_msg void OnViewPlacementGrid();
	void SaveOptionsToRegistry();
	void ReadOptionsFromRegistry();
	void CollectOptionsStrings(CArray<CString> &arr);
	// end CPT
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FREEPCBDOC_H__A00395C2_2CF4_4902_9C7B_CBB16DB58836__INCLUDED_)
