// FreePcbDoc.h : interface of the CFreePcbDoc class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_FREEPCBDOC_H__A00395C2_2CF4_4902_9C7B_CBB16DB58836__INCLUDED_)
#define AFX_FREEPCBDOC_H__A00395C2_2CF4_4902_9C7B_CBB16DB58836__INCLUDED_

#include "stdafx.h"

#pragma once
#define CPT2 0

#include "PcbFont.h"
#include "SMfontutil.h"
#include "SMcharacter.h"
#include "FootprintLib.h"
#include "DlgDRC.h"
#include "DesignRules.h"
//#include "QAFDebug.h"
#include "PcbItem.h"
#include "NetListNew.h"
#include "PartListNew.h"
#include "TextListNew.h"

class CFreePcbDoc;
class CFreePcbView;
class cpcb_item;
template <class T> class carray;

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
	// Collections of pcb-items and drawing elements:
	CDisplayList * m_dlist;		// current display list.  CPT2: new.  Sometimes this is equal to m_dlist_pcb, sometimes m_dlist_fp.
	CDisplayList * m_dlist_pcb; // display list for main editor
	CDisplayList * m_dlist_fp;	// display list for footprint editor
	cnetlist *m_nlist;			// CPT2.  Was CNetList, now cnetlist
	cpartlist *m_plist;			// CPT2.  Was CPartList, now cpartlist
	ctextlist * m_tlist;		// CPT2.  Was CTextList, now ctextlist
	carray<cboard> boards;
	carray<csmcutout> smcutouts;
	carray<coutline> outlines;		// CPT2 maybe...  Still have to figure out the memory management of entities within CShapes.
	cdrelist * m_drelist;			// CPT2.  Was DREList, now cdrelist
	carray<cpcb_item> items;		// CPT2.  Master list of all created pcb-items.  GarbageCollect() will go through this list and clean up now and then.
	carray<cpcb_item> redraw;		// CPT2 r313.  My latest-n-greatest system for undrawing and redrawing (see notes.txt).

	CFreePcbView * m_view;		// pointer to CFreePcbView 
	CDlgLog * m_dlg_log;
	SMFontUtil * m_smfontutil;	// Hershey font utility
	int m_file_close_ret;		// return value from OnFileClose() dialog
	CFootLibFolderMap m_footlibfoldermap;
	CMapStringToPtr m_footprint_cache_map;	// map of footprints cached in memory

	double m_version;			// version number, such as "1.105"
	double m_file_version;		// the oldest version of FreePCB that can read
								// files created with this version
	double m_read_version;		// the version from the project file
	int m_WindowsMajorVersion;	// CPT2 added, seemed helpful 
	BOOL bNoFilesOpened;		// TRUE if no files have been opened
	CShape * m_edit_footprint;	// Set if we're editing the footprint of a selected part.  CPT2 was BOOL, made it more informative...
	BOOL m_project_open;		// FALSE if no project open
	BOOL m_project_modified;	// FALSE if project not modified since loaded
	BOOL m_project_modified_since_autosave;	// FALSE if project not modified since loaded
	BOOL m_footprint_modified;	// FALSE if the footprint being edited has not changed
	BOOL m_footprint_name_changed;	// TRUE if the footprint being edited has had its name changed
	CString m_window_title;		// window title for PCB editor
	CString m_fp_window_title;	// window title for footprint editor
	CString m_name;				// project name
	CString m_app_dir;			// application directory (full path) 
	CString m_defaultcfg_dir;	// CPT2 new.  A value read off the registry;  then we look in that dir for our default.cfg options file.
	CString m_lib_dir;			// path to default library folder (may be relative)
	CString m_full_lib_dir;		// full path to default library folder
	CString m_parent_folder;	// path to parent of project folders (may be relative)
	CString m_path_to_folder;	// path to project folder
	CString m_pcb_filename;		// name of project file
	CString m_pcb_full_path;	// full path to project file
	CString m_cam_full_path;	// full path to CAM file folder
	CString m_netlist_full_path;	// last netlist loaded

	// undo and redo stacks and state
	CArray<cundo_item*> m_undo_items;		// CPT2 new undo system.  Altered items accumulate in here during a user operation, then get combined
											// into a new cundo_record by FinishUndoRecord().
	CArray<cundo_record*> m_undo_records;	// CPT2 The main list that both undo and redo operations refer to.
	int m_undo_pos;							// CPT2 Offset into m_undo_items, marking the dividing line between undo and redo records.
	int m_undo_last_uid;					// CPT2 FinishUndoRecord() refers to this to determine which cpcb_items are newly created since it was last invoked
	enum { UNDO_MAX = 100 };

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

	// pseudo-clipboard.  CPT2 changed to the new object types:
	cpartlist *clip_plist;
	cnetlist *clip_nlist;
	ctextlist *clip_tlist;
	carray<csmcutout> clip_smcutouts;
	carray<cboard> clip_boards;

	// define world units for CDisplayList
	int m_pcbu_per_wu;

	// grids and units for pcb editor
	// int m_units;					// MM or MIL					// CPT2 Think I'll abandon this and use CCommonView::m_units instead
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
	bool m_bHighlightNet;	// AMW
	// end CPT

	//DRC limits
	DesignRules m_dr;

	// Brian's stuff
	int m_thermal_clearance;


protected: // create from serialization only
	CFreePcbDoc();
	DECLARE_DYNCREATE(CFreePcbDoc)

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);

// Implementation
	virtual ~CFreePcbDoc();
	void OnTimer();
	BOOL FileOpen( LPCTSTR fn, BOOL bLibrary=FALSE );
	int FileClose();
	void FileLoadLibrary( LPCTSTR pathname );
	void ProjectModified( BOOL flag, BOOL bCombineWithPreviousUndo = false );		// CPT2 changed second arg
	void InitializeNewProject();
	void SendInitialUpdate();
	void ReadFootprints( CStdioFile * pcb_file, 
		CMapStringToPtr * cache_map=NULL,
		BOOL bFindSection=TRUE );
	int WriteFootprints( CStdioFile * file, CMapStringToPtr * cache_map=NULL );
	CShape * GetFootprintPtr( CString name );
	void MakeLibraryMaps( CString * fullpath );
	void ReadBoardOutline( CStdioFile * pcb_file);
	void WriteBoardOutline( CStdioFile * pcb_file, carray<cboard> *boards = NULL );
	void ReadSolderMaskCutouts( CStdioFile * pcb_file );
	void WriteSolderMaskCutouts( CStdioFile * pcb_file, carray<csmcutout> *smcutouts = NULL );
	void ReadOptions( CStdioFile * pcb_file );
	void WriteOptions( CStdioFile * pcb_file );
	int ImportNetlist( CStdioFile * file, UINT flags, 
						partlist_info * pli, netlist_info * nli );
	int ImportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pli, netlist_info * nli );
	int ExportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pli, netlist_info * nli );
	void ImportSessionFile( CString * filepath, CDlgLog * log=NULL, BOOL bVerbose=TRUE );
	void OnFileAutoOpen( LPCTSTR fn );
	BOOL FileSave( CString * folder, CString * filename, 
		CString * old_folder, CString * old_filename,
		BOOL bBackup=TRUE );
	BOOL AutoSave(); 
	// I created the following routine while struggling to understand the misbehavior of the CFileDialog interface.  (Turns out the solution is
	// just to build in release mode rather than debug mode --- debug object code and common control routines apparently conflict pretty badly.)
	// Anyway, during the process I figured out (with considerable difficulty) how to use the new Vista-style common controls.  Though it turns out not
	// to be necessary, I'm leaving the code in as a comment, so that we can potentially use it one day...
	// bool GetFileName(bool bSave, CString initial, int titleRsrc, int filterRsrc, WCHAR *defaultExt, WCHAR *result, int *offFileName = NULL);  
	void SetFileLayerMap( int file_layer, int layer );
	void PurgeFootprintCache();

	void ResetUndoState();											// CPT2:  Reused the name, but transformed it into the new system.
	void FinishUndoRecord(bool bCombineWithPrevious=false);			// CPT2:  Critical part of the new architecture (see UndoNew.cpp)
	void CreateMoveOriginUndoRecord( int x_off, int y_off );		// CPT2:  Reused the name.
	void UndoNoRedo();												// CPT2:  Used occasionally, e.g. when user aborts a just-started operation 
																	// like dragging a stub

	// CPT2 DRC() used to be in CPartList, but I thought it made more sense here.  I also broke DRC() into helper routines for readability.
	void DRC( int units, BOOL check_unrouted, DesignRules * dr );
	void DRCPin( cpin2 *pin, int units, DesignRules *dr );
	void DRCPinsAndCopperGraphics( cpart2 *part1, cpart2 *part2, int units, DesignRules *dr );
	void DRCPinAndPin( cpin2 *pin1, cpin2 *pin2, int units, DesignRules *dr, int clearance );
	void DRCArea( carea2 *a, int units, DesignRules *dr );
	void DRCConnect( cconnect2 *c, int units, DesignRules *dr );
	void DRCConnectAndConnect( cconnect2 *c1, cconnect2 *c2, int units, DesignRules *dr, int clearance );
	void DRCAreaAndArea( carea2 *a1, carea2 *a2, int units, DesignRules *dr );
	void DRCSegmentAndVia( cseg2 *seg, cvertex2 *vtx, int units, DesignRules *dr );
	void DRCViaAndVia(cvertex2 *vtx1, cvertex2 *vtx2, int units, DesignRules *dr);
	void DRCUnrouted(int units);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


// Generated message map functions
public:
	//{{AFX_MSG(CFreePcbDoc)
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
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
	afx_msg void OnFileLoadLibrary();
	afx_msg void OnFileSaveLibrary();
	// CPT
	afx_msg void OnToolsPreferences();
	afx_msg void OnViewRoutingGrid();
	afx_msg void OnViewVisibleGrid();
	afx_msg void OnViewPlacementGrid();
	void CollectOptionsStrings(CArray<CString> &arr);
	// CPT2
	void Redraw();												// CPT2 r313, latest-n-greatest system for undrawing/redrawing (see notes.txt)
	void GarbageCollect();
	void CheckDefaultCfg();
	// end CPT
};

#endif // !defined(AFX_FREEPCBDOC_H__A00395C2_2CF4_4902_9C7B_CBB16DB58836__INCLUDED_)
