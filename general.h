// general.h --- consolidated header file with general purpose constants:  layers, flags, etc.  Combines old layers.h, flags.h, + other stuff

#pragma once

#define MAX_LAYERS 32

enum
{
	// layout layers
	LAY_SELECTION = 0,
	LAY_BACKGND,
	LAY_VISIBLE_GRID,
	LAY_HILITE,
	LAY_DRC_ERROR,
	LAY_BOARD_OUTLINE,
	LAY_RAT_LINE,
	LAY_SILK_TOP,
	LAY_SILK_BOTTOM,
	LAY_SM_TOP,
	LAY_SM_BOTTOM,
	LAY_PAD_THRU,
	LAY_TOP_COPPER,
	LAY_BOTTOM_COPPER,
	// invisible layers
	LAY_MASK_TOP = -100,	
	LAY_MASK_BOTTOM = -101,
	LAY_PASTE_TOP = -102,
	LAY_PASTE_BOTTOM = -103
};

enum
{
	// footprint layers
	LAY_FP_SELECTION = 0,
	LAY_FP_BACKGND,
	LAY_FP_VISIBLE_GRID,
	LAY_FP_HILITE,
	LAY_FP_SILK_TOP,
	LAY_FP_SILK_BOTTOM,
	LAY_FP_CENTROID,
	LAY_FP_DOT,
	LAY_FP_PAD_THRU,
	LAY_FP_TOP_MASK,
	LAY_FP_TOP_PASTE,
	LAY_FP_BOTTOM_MASK,
	LAY_FP_BOTTOM_PASTE,
	LAY_FP_TOP_COPPER,
	LAY_FP_INNER_COPPER,
	LAY_FP_BOTTOM_COPPER,
	NUM_FP_LAYERS
};

//  CPT:  the following tables are used when reading/writing Gerber and .fpc files.  
//  But they are not used when UI elements are being drawn
//  (the string resource table is used instead)

static char layer_str[32][64] = 
{ 
	"selection",
	"background",
	"visible grid",
	"highlight",
	"DRC error",
	"board outline",
	"rat line",
	"top silk",
	"bottom silk",
	"top sm cutout",
	"bot sm cutout",
	"thru pad",
	"top copper",
	"bottom copper",
	"inner 1",
	"inner 2",
	"inner 3",
	"inner 4",
	"inner 5",
	"inner 6",
	"inner 7",
	"inner 8",
	"inner 9",
	"inner 10",
	"inner 11",
	"inner 12",
	"inner 13",
	"inner 14",
	"inner 15",
	"inner 16",
	"undefined",
	"undefined"
}; 

static char fp_layer_str[NUM_FP_LAYERS][64] = 
{ 
	"selection",
	"background",
	"visible grid",
	"highlight",
	"top silk",
	"bottom_silk",
	"centroid",
	"adhesive",
	"thru pad",
	"top mask",
	"top paste",
	"bottom mask",
	"bottom paste",
	"top copper",
	"inner",
	"bottom"
};

static char layer_char[17] = "12345678QWERTYUI";


enum {
	IMPORT_FROM_NETLIST_FILE	= 0x1,
	IMPORT_PARTS				= 0x2,
	IMPORT_NETS					= 0x4,
	KEEP_PARTS_AND_CON			= 0x8,
	KEEP_PARTS_NO_CON			= 0x10,
	KEEP_FP						= 0x20,
	KEEP_NETS					= 0x40,
	SAVE_BEFORE_IMPORT			= 0x400,
	NO_EXISTING_PARTS			= 0x800,		// CPT2 new
	NO_EXISTING_NETS			= 0x1000,		// ditto
	SYNC_NETLIST_FILE			= 0x2000,		// ditto
	FORCE_FILE_SYNC_CHECK	    = 0x4000,		// ditto
	DONT_LOAD_SHAPES			= 0x8000		// ditto
};

enum {
	EXPORT_PARTS		= 0x1,
	EXPORT_NETS			= 0x2,
	EXPORT_VALUES		= 0x4
};

enum {
	// netlist return flags
	FOOTPRINTS_NOT_FOUND = 1,
	NOT_PADSPCB_FILE
};

enum {
	// flags returned by DlgGroupPaste
	PASTE_DRAG					= 0x1,
	PASTE_PARTS_NEXT_AVAIL		= 0x2,
	PASTE_PARTS_RETAIN			= 0x4,
	PASTE_PARTS_OFFSET			= 0x8,
	PASTE_NETS_IGNORE_TRACELESS = 0x10,
	PASTE_NETS_MERGE_ALL		= 0x20,
	PASTE_NETS_MERGE_SOME		= 0x40,
	PASTE_NETS_RENAME_G			= 0x80
};
enum 
{
	// Warning message id's
	RATLINE_WARNING = 0,
	FP_PASTE_WARNING,
	SELF_INTERSECTION_WARNING,
	SELF_INTERSECTION_ARC_WARNING,
	INTERSECTION_WARNING,
	INTERSECTION_ARC_WARNING,
	HEADER_28_MIL_WARNING,
	CANT_OPEN_LIBRARY_WARNING,
	COPPER_CLEARANCE_WARNING,
	AREA_START_TRACE_WARNING,
	CONNECT_TO_VTX_WARNING,
	NUM_WARNINGS
};


enum
{
	// CPT2 Logging codes
	LOG_KEY,
	LOG_LEFT_CLICK,
	LOG_RIGHT_CLICK,
	LOG_ON_ADD_AREA,
	LOG_ON_ADD_BOARD_OUTLINE,
	LOG_ON_ADD_NET,	
	LOG_ON_ADD_PART,
	LOG_ON_ADD_SIMILAR_AREA,
	LOG_ON_ADD_SOLDER_MASK_CUTOUT,
	LOG_ON_AREA_EDIT,
	LOG_ON_AREA_START_TRACE,
	LOG_ON_CHECK_PARTS_AND_NETS,
	LOG_ON_CLEAR_DRC,
	LOG_CHANGE_TRACE_LAYER,
	LOG_SET_WIDTH,
	LOG_ON_DELETE_CONNECTION,
	LOG_ON_DRC,
	LOG_ON_EXTERNAL_CHANGE_FP,
	LOG_ON_GROUP_COPY,
	LOG_ON_GROUP_CUT,
	LOG_ON_GROUP_DELETE,
	LOG_ON_GROUP_MOVE,
	LOG_ON_GROUP_PASTE,
	LOG_ON_GROUP_ROTATE,
	LOG_ON_GROUP_SAVE_TO_FILE,
	LOG_ON_OPTIMIZE,
	LOG_ON_PAD_ADD_TO_NET,
	LOG_ON_PAD_DETACH_FROM_NET,
	LOG_ON_PAD_OPTIMIZE,
	LOG_ON_PAD_START_RATLINE,
	LOG_ON_PAD_START_TRACE,
	LOG_ON_PART_CHANGE_SIDE,
	LOG_ON_PART_DELETE,
	LOG_ON_PART_GLUE,
	LOG_ON_PART_MOVE,
	LOG_ON_PART_OPTIMIZE,
	LOG_ON_PART_PROPERTIES,
	LOG_ON_PART_REF_PROPERTIES,
	LOG_ON_PART_ROTATE,
	LOG_ON_PART_UNGLUE,
	LOG_ON_PART_VALUE_PROPERTIES,
	LOG_ON_POLY_ADD_CUTOUT,
	LOG_ON_POLY_CORNER_DELETE,
	LOG_ON_POLY_CORNER_EDIT,
	LOG_ON_POLY_CORNER_MOVE,
	LOG_ON_POLY_DELETE,
	LOG_ON_POLY_DELETE_CUTOUT,
	LOG_ON_POLY_SIDE_ADD_CORNER,
	LOG_ON_POLY_SIDE_CONVERT,
	LOG_ON_RATLINE_CHANGE_END_PIN,
	LOG_ON_RATLINE_COMPLETE,
	LOG_ON_RATLINE_LOCK_CONNECTION,
	LOG_ON_RATLINE_ROUTE,
	LOG_ON_RATLINE_UNLOCK_CONNECTION,
	LOG_ON_REF_MOVE,
	LOG_ON_REF_PROPERTIES,
	LOG_ON_REF_ROTATE,
	LOG_ON_REPEAT_DRC,
	LOG_ON_SEGMENT_ADD_VERTEX,
	LOG_ON_SEGMENT_DELETE,
	LOG_ON_SEGMENT_MOVE,
	LOG_ON_SEGMENT_UNROUTE,
	LOG_ON_SM_EDIT,
	LOG_ON_TEE_DELETE,
	LOG_ON_TEE_MOVE,
	LOG_ON_TEE_PROPERTIES,
	LOG_ON_TEXT_ADD,
	LOG_ON_TEXT_DELETE,
	LOG_ON_TEXT_EDIT,
	LOG_ON_TEXT_MOVE,
	LOG_ON_TEXT_ROTATE,
	LOG_ON_TOOLS_MOVE_ORIGIN,
	LOG_ON_UNROUTE_TRACE,
	LOG_ON_VERTEX_ADD_VIA,
	LOG_ON_VERTEX_DELETE,
	LOG_ON_VERTEX_MOVE,
	LOG_ON_VERTEX_PROPERTIES,
	LOG_ON_VERTEX_REMOVE_VIA,
	LOG_ON_VERTEX_START_RATLINE,
	LOG_ON_VERTEX_START_TRACE,
	LOG_ON_EDIT_PASTE_FROM_FILE,
	LOG_ON_EDIT_REDO,
	LOG_ON_EDIT_UNDO,
	LOG_ON_PROJECT_NETLIST,
	LOG_ON_PROJECT_PARTLIST,
	LOG_ON_TOOLS_CHECK_CONNECTIVITY,
	LOG_ON_TOOLS_CHECK_COPPER_AREAS,
	LOG_ON_TOOLS_CHECK_PARTS_AND_NETS,
	LOG_ON_TOOLS_CHECK_TRACES,
};

