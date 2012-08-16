// DlgWizQuad.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgWizQuad.h"
#include "DlgSaveFootprint.h"

#define MAX_BGA_ROWS 60
#define MAX_PINS MAX_BGA_ROWS*MAX_BGA_ROWS

static char bga_row_name[MAX_BGA_ROWS][3] =
{
	"A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "L", "M", "N", "P", "R", "T", "U", "V", "W", "Y",
	"AA","AB","AC","AD","AE","AF","AG","AH","AJ","AK","AL","AM","AN","AP","AR","AT","AU","AV","AW","AY",
	"BA","BB","BC","BD","BE","BF","BG","BH","BJ","BK","BL","BM","BN","BP","BR","BT","BU","BV","BW","BY"
};

//globals
extern CString gLastFileName;		// last file name imported

// CDlgWizQuad dialog

IMPLEMENT_DYNAMIC(CDlgWizQuad, CDialog)
CDlgWizQuad::CDlgWizQuad(CWnd* pParent /*=NULL*/) 
	: CDialog(CDlgWizQuad::IDD, pParent)
{
}

CDlgWizQuad::~CDlgWizQuad()
{
}

void CDlgWizQuad::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BMP_QUAD, m_bmp_quad);
	DDX_Control(pDX, IDC_RADIO_C1, m_radio_C1);
	DDX_Control(pDX, IDC_RADIO_G1, m_radio_G1);
	DDX_Control(pDX, IDC_RADIO_Z1, m_radio_Z1);
	DDX_Control(pDX, IDC_RADIO_C2, m_radio_C2);
	DDX_Control(pDX, IDC_RADIO_G2, m_radio_G2);
	DDX_Control(pDX, IDC_RADIO_Z2, m_radio_Z2);
	DDX_Control(pDX, IDC_EDIT_C1, m_edit_C1);
	DDX_Control(pDX, IDC_EDIT_G1, m_edit_G1);
	DDX_Control(pDX, IDC_EDIT_Z1, m_edit_Z1);
	DDX_Control(pDX, IDC_EDIT_C2, m_edit_C2);
	DDX_Control(pDX, IDC_EDIT_G2, m_edit_G2);
	DDX_Control(pDX, IDC_EDIT_Z2, m_edit_Z2);
	DDX_Control(pDX, IDC_EDIT_E, m_edit_E);
	DDX_Control(pDX, IDC_COMBO_WIZ_SHAPE, m_combo_shape);
	DDX_Control(pDX, IDC_COMBO_WIZ_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_EDIT_WIZ_Y, m_edit_y);
	DDX_Control(pDX, IDC_EDIT_WIZ_HD, m_edit_hd);
	DDX_Control(pDX, IDC_EDIT_WIZ_X, m_edit_x);
	DDX_Control(pDX, IDC_COMBO_PIN1, m_combo_pin1);
	DDX_Control(pDX, IDC_EDIT_WIZ_NPINS, m_edit_npins);
	DDX_Control(pDX, IDC_EDIT_WIZ_HPINS, m_edit_hpins);
	DDX_Control(pDX, IDC_EDIT_WIZ_VPINS, m_edit_vpins);
	DDX_Control(pDX, IDC_EDIT_WIZ_NAME, m_edit_name);
	DDX_Control(pDX, IDC_COMBO_WIZ_TYPE, m_combo_type);
	DDX_Control(pDX, IDC_WIZ_BUTTON_SAVE, m_button_save);
	DDX_Control(pDX, ID_WIZ_BUTTON_EXIT, m_button_exit);
	DDX_Control(pDX, IDC_BUTTON_PREVIEW, m_button_preview);
	DDX_Control(pDX, IDC_STATIC_WIZ_PREVIEW, m_preview);
	DDX_Control(pDX, IDC_EDIT_RAD, m_edit_radius);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		if( !m_enable_save )
		{
			CString s ((LPCSTR) IDS_Done3);
			m_button_save.SetWindowText( s );
			s.LoadStringA(IDS_Cancel);
			m_button_exit.SetWindowText( s );
		}
		m_bmp.LoadBitmap( IDB_BITMAP_QUAD );
		m_bmp_quad.SetBitmap( m_bmp );
		m_radio_C1.SetCheck(1);
		m_radio_C2.SetCheck(1);
		m_edit_G1.EnableWindow(0);
		m_edit_Z1.EnableWindow(0);
		m_edit_G2.EnableWindow(0);
		m_edit_Z2.EnableWindow(0);
		CString s;
		for (int i=0; i<8; i++) 
			s.LoadStringA(IDS_WizPinShapes+i),
			m_combo_shape.InsertString( -1, s);
		m_combo_shape.SetCurSel(4);
		m_shape = RECT;
		m_edit_y.EnableWindow(1);
		m_edit_radius.EnableWindow(0);
		m_combo_units.InsertString( -1, "mils" );
		m_combo_units.InsertString( -1, "mm" );
		m_combo_units.SetCurSel(0);
		m_units = MIL;
		m_mult = NM_PER_MIL;
		for (int i=0; i<3; i++) 
			s.LoadStringA(IDS_WizPin1+i),
			m_combo_pin1.InsertString( -1, s);
		m_combo_pin1.SetCurSel(0);
		OnCbnSelchangeComboPin1();
		m_pin1 = TOP_CENTER;
		for (int i=0; i<8; i++) 
			s.LoadStringA(IDS_WizType+i),
			m_combo_type.InsertString( -1, s);
		m_combo_type.SetCurSel(0);
		OnCbnSelchangeComboWizType();
		m_npins = 0;
		m_edit_npins.SetWindowText( "" );
		m_hpins = 0;
		m_edit_hpins.SetWindowText( "" );
		m_vpins = 0;
		m_edit_vpins.SetWindowText( "" );
		m_hd = 0;
		m_x = 0;
		m_y = 0;
		m_r = 0;
		m_e = 0;
		m_c1 = 0;
		m_g1 = 0;
		m_z1 = 0;
		m_c2 = 0;
		m_g2 = 0;
		m_z2 = 0;
	}
}


void CDlgWizQuad::Initialize( CMapStringToPtr * shape_cache_map,
							CFootLibFolderMap * footlibfoldermap, 
							BOOL enable_save,
							CDlgLog * log )
{
	extern CFreePcbApp theApp;
	m_footprint = new CShape (theApp.m_doc);
	m_enable_save = enable_save;
	m_footprint_cache_map = shape_cache_map;
	m_footlibfoldermap = footlibfoldermap;
	m_dlg_log = log;
}


BEGIN_MESSAGE_MAP(CDlgWizQuad, CDialog)
	ON_BN_CLICKED(IDC_RADIO_C1, OnBnClickedRadioC1)
	ON_BN_CLICKED(IDC_RADIO_G1, OnBnClickedRadioG1)
	ON_BN_CLICKED(IDC_RADIO_Z1, OnBnClickedRadioZ1)
	ON_BN_CLICKED(IDC_RADIO_C2, OnBnClickedRadioC2)
	ON_BN_CLICKED(IDC_RADIO_G2, OnBnClickedRadioG2)
	ON_BN_CLICKED(IDC_RADIO_Z2, OnBnClickedRadioZ2)
	ON_CBN_SELCHANGE(IDC_COMBO_WIZ_SHAPE, OnCbnSelchangeComboWizShape)
	ON_EN_CHANGE(IDC_EDIT_WIZ_NPINS, OnEnChangeEditWizNpins)
	ON_EN_CHANGE(IDC_EDIT_WIZ_HPINS, OnEnChangeEditWizHpins)
	ON_EN_CHANGE(IDC_EDIT_WIZ_X, OnEnChangeEditWizX)
	ON_EN_CHANGE(IDC_EDIT_WIZ_Y, OnEnChangeEditWizY)
	ON_EN_CHANGE(IDC_EDIT_RAD, OnEnChangeEditWizR)
	ON_EN_CHANGE(IDC_EDIT_E, OnEnChangeEditE)
	ON_EN_CHANGE(IDC_EDIT_C1, OnEnChangeEditC1)
	ON_EN_CHANGE(IDC_EDIT_G1, OnEnChangeEditG1)
	ON_EN_CHANGE(IDC_EDIT_Z1, OnEnChangeEditZ1)
	ON_EN_CHANGE(IDC_EDIT_C2, OnEnChangeEditC2)
	ON_EN_CHANGE(IDC_EDIT_G2, OnEnChangeEditG2)
	ON_EN_CHANGE(IDC_EDIT_Z2, OnEnChangeEditZ2)
	ON_CBN_SELCHANGE(IDC_COMBO_WIZ_TYPE, OnCbnSelchangeComboWizType)
	ON_CBN_SELCHANGE(IDC_COMBO_WIZ_UNITS, OnCbnSelchangeComboWizUnits)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, OnBnClickedButtonSave)
	ON_CBN_SELCHANGE(IDC_COMBO_PIN1, OnCbnSelchangeComboPin1)
	ON_EN_CHANGE(IDC_EDIT_WIZ_HD, OnEnChangeEditWizHd)
	ON_BN_CLICKED(ID_WIZ_BUTTON_EXIT, OnBnClickedWizButtonExit)
	ON_BN_CLICKED(IDC_BUTTON_PREVIEW, OnBnClickedButtonPreview)
END_MESSAGE_MAP()


// CDlgWizQuad message handlers

void CDlgWizQuad::OnBnClickedRadioC1()
{
	m_edit_C1.EnableWindow(1);
	m_edit_G1.EnableWindow(0);
	m_edit_Z1.EnableWindow(0);
}

void CDlgWizQuad::OnBnClickedRadioG1()
{
	m_edit_C1.EnableWindow(0);
	m_edit_G1.EnableWindow(1);
	m_edit_Z1.EnableWindow(0);
}

void CDlgWizQuad::OnBnClickedRadioZ1()
{
	m_edit_C1.EnableWindow(0);
	m_edit_G1.EnableWindow(0);
	m_edit_Z1.EnableWindow(1);
}

void CDlgWizQuad::OnBnClickedRadioC2()
{
	m_edit_C2.EnableWindow(1);
	m_edit_G2.EnableWindow(0);
	m_edit_Z2.EnableWindow(0);
}

void CDlgWizQuad::OnBnClickedRadioG2()
{
	m_edit_C2.EnableWindow(0);
	m_edit_G2.EnableWindow(1);
	m_edit_Z2.EnableWindow(0);
}

void CDlgWizQuad::OnBnClickedRadioZ2()
{
	m_edit_C2.EnableWindow(0);
	m_edit_G2.EnableWindow(0);
	m_edit_Z2.EnableWindow(1);
}

void CDlgWizQuad::OnCbnSelchangeComboWizShape()
{
	CString str, x_str;
	m_combo_shape.GetWindowText( str );
	enum { shpRound=0, shpSquare, shpPin1Square, shpOctagon, 
		   shpRectangular, shpRoundedRect, shpOval, shpNone };
	int index = m_combo_shape.GetCurSel( );
	if( index==shpRectangular )
	{
		m_shape = RECT;
		m_edit_x.EnableWindow(1);
		m_edit_y.EnableWindow(1);
		m_edit_radius.EnableWindow(0);
		m_edit_radius.SetWindowText( "0" );
	}
	else if( index==shpOval )
	{
		m_shape = OVAL;
		m_edit_x.EnableWindow(1);
		m_edit_y.EnableWindow(1);
		m_edit_radius.EnableWindow(0);
		m_edit_radius.SetWindowText( "0" );
	}
	else if( index==shpRoundedRect )
	{
		m_shape = nliECT;
		m_edit_x.EnableWindow(1);
		m_edit_y.EnableWindow(1);
		m_edit_radius.EnableWindow(1);
	}
	else if( index==shpNone )
	{
		m_shape = NONE;
		m_edit_x.EnableWindow(0);
		m_edit_y.EnableWindow(0);
		m_edit_radius.EnableWindow(0);
		m_edit_x.SetWindowText( "0" );
		m_edit_y.SetWindowText( "0" );
		m_edit_radius.SetWindowText( "0" );
	}
	else
	{
		if( index==shpPin1Square )
			m_shape = SQ1;
		else if( index==shpRound )
			m_shape = ROUND;
		else if( index==shpOctagon )
			m_shape = OCTAGON;
		else if( index==shpSquare )
			m_shape = SQUARE;
		m_edit_x.EnableWindow(1);
		m_edit_y.EnableWindow(0);
		m_edit_radius.EnableWindow(0);
		m_edit_y.SetWindowText( x_str );
		m_edit_radius.SetWindowText( "0" );
	}
	OnEnChangeEditWizX();	// in case we need to update m_y
	ReviseDependentVariables();
}

void CDlgWizQuad::OnEnChangeEditWizNpins()
{
	CString str;
	m_edit_npins.GetWindowText( str );
	m_npins = my_atoi( &str );
	if( m_type == QUAD )
		m_vpins = (m_npins - 2*m_hpins)/2;
	else if(  m_type == HDR1 || m_type == HDR2 )
	{
		if( m_hpins == 0 )
			m_vpins = m_npins;
		else
			m_vpins = m_npins/m_hpins;
	}
	else if( m_type == DIP || m_type == EDGE || m_type == EDGE2 )
	{
		m_hpins = m_npins/2;
		str.Format( "%d", m_hpins );
		if( m_hpins >= 0 )
			m_edit_hpins.SetWindowText( str );
	}
	else if( m_type == SIP )
	{
		m_hpins = m_npins;
	}
	if( m_type == SIP || m_type == DIP )
	{
		str.Format( "%d", m_hpins );
		if( m_hpins >= 0 )
			m_edit_hpins.SetWindowText( str );
	}
	str.Format( "%d", m_vpins );
	if( m_vpins >= 0 )
		m_edit_vpins.SetWindowText( str );
}

void CDlgWizQuad::OnEnChangeEditWizHpins()
{
	CString str;
	m_edit_hpins.GetWindowText( str );
	m_hpins = my_atoi( &str );
	if( m_type == SIP || m_type == DIP )
		m_vpins = 0;
	else if( m_type == QUAD )
		m_vpins = (m_npins - 2*m_hpins)/2;
	else if( m_type == HDR1 || m_type == HDR2 || m_type == BGA )
	{
		if( m_hpins == 0 )
			m_vpins = m_npins;
		else
			m_vpins = m_npins/m_hpins;
	}
	str.Format( "%d", m_vpins );
	if( m_vpins >= 0 )
		m_edit_vpins.SetWindowText( str );
}

void CDlgWizQuad::OnEnChangeEditWizX()
{
	CString str;
	m_edit_x.GetWindowText( str );
	m_x = atof( str ) * m_mult;
	if( m_shape != RECT && m_shape != nliECT && m_shape != OVAL ) 
	{
		m_edit_y.SetWindowText( str );
		m_y = m_x;
		ReviseDependentVariables();
	}
}

void CDlgWizQuad::OnEnChangeEditWizY()
{
	CString str;
	m_edit_y.GetWindowText( str );
	m_y = atof( str ) * m_mult;
	ReviseDependentVariables();
}

void CDlgWizQuad::OnEnChangeEditWizR()
{
	CString str;
	m_edit_radius.GetWindowText( str );
	m_r = atof( str ) * m_mult;
	ReviseDependentVariables();
}

void CDlgWizQuad::OnEnChangeEditE()
{
	CString str;
	m_edit_E.GetWindowText( str );
	m_e = atof( str ) * m_mult;
}

void CDlgWizQuad::OnEnChangeEditC1()
{
	if( m_radio_C1.GetCheck() )
	{
		CString str;
		m_edit_C1.GetWindowText( str );
		m_c1 = atof( str ) * m_mult;
		ReviseDependentVariables();
	}
}

void CDlgWizQuad::OnEnChangeEditG1()
{
	if( m_radio_G1.GetCheck() )
	{
		CString str;
		m_edit_G1.GetWindowText( str );
		m_g1 = atof( str ) * m_mult;
		ReviseDependentVariables();
	}
}

void CDlgWizQuad::OnEnChangeEditZ1()
{
	if( m_radio_Z1.GetCheck() )
	{
		CString str;
		m_edit_Z1.GetWindowText( str );
		m_z1 = atof( str ) * m_mult;
		ReviseDependentVariables();
	}
}

void CDlgWizQuad::OnEnChangeEditC2()
{
	if( m_radio_C2.GetCheck() )
	{
		CString str;
		m_edit_C2.GetWindowText( str );
		m_c2 = atof( str ) * m_mult;
		ReviseDependentVariables();
	}
}

void CDlgWizQuad::OnEnChangeEditG2()
{
	if( m_radio_G2.GetCheck() )
	{
		CString str;
		m_edit_G2.GetWindowText( str );
		m_g2 = atof( str ) * m_mult;
		ReviseDependentVariables();
	}
}

void CDlgWizQuad::OnEnChangeEditZ2()
{
	if( m_radio_Z2.GetCheck() )
	{
		CString str;
		m_edit_Z2.GetWindowText( str );
		m_z2 = atof( str ) * m_mult;
		ReviseDependentVariables();
	}
}

// revise dependent variables from C1, G1, Z1, C2, G2, Z2
//
void CDlgWizQuad::ReviseDependentVariables()
{
	CString str;
	if( m_type == DIP || m_type == QUAD )
	{
		if( m_radio_C1.GetCheck() )
		{
			m_g1 = m_c1 - m_y;
			FormatDimensionString( &str, m_g1, m_units );
			m_edit_G1.SetWindowText( str );
			m_z1 = m_c1 + m_y;
			FormatDimensionString( &str, m_z1, m_units );
			m_edit_Z1.SetWindowText( str );
		}
		else if( m_radio_G1.GetCheck() )
		{
			m_c1 = m_g1 + m_y;
			FormatDimensionString( &str, m_c1, m_units );
			m_edit_C1.SetWindowText( str );
			m_z1 = m_g1 + 2*m_y;
			FormatDimensionString( &str, m_z1, m_units );
			m_edit_Z1.SetWindowText( str );
		}
		else if( m_radio_Z1.GetCheck() )
		{
			m_c1 = m_z1 - m_y;
			FormatDimensionString( &str, m_c1, m_units );
			m_edit_C1.SetWindowText( str );
			m_g1 = m_z1 - 2*m_y;
			FormatDimensionString( &str, m_g1, m_units );
			m_edit_G1.SetWindowText( str );
		}
	}
	if( m_type == QUAD )
	{
		if( m_radio_C2.GetCheck() )
		{
			m_g2 = m_c2 - m_y;
			FormatDimensionString( &str, m_g2, m_units );
			m_edit_G2.SetWindowText( str );
			m_z2 = m_c2 + m_y;
			FormatDimensionString( &str, m_z2, m_units );
			m_edit_Z2.SetWindowText( str );
		}
		else if( m_radio_G2.GetCheck() )
		{
			m_c2 = m_g2 + m_y;
			FormatDimensionString( &str, m_c2, m_units );
			m_edit_C2.SetWindowText( str );
			m_z2 = m_g2 + 2*m_y;
			FormatDimensionString( &str, m_z2, m_units );
			m_edit_Z2.SetWindowText( str );
		}
		else if( m_radio_Z2.GetCheck() )
		{
			m_c2 = m_z2 - m_y;
			FormatDimensionString( &str, m_c2, m_units );
			m_edit_C2.SetWindowText( str );
			m_g2 = m_z2 - 2*m_y;
			FormatDimensionString( &str, m_g2, m_units );
			m_edit_G2.SetWindowText( str );
		}
	}
}

void CDlgWizQuad::OnCbnSelchangeComboWizType()
{
	CString str;
	m_type = m_combo_type.GetCurSel();
	if( m_type == QUAD )
	{
		m_bmp.DeleteObject();
		m_bmp.LoadBitmap( IDB_BITMAP_QUAD );
		m_bmp_quad.SetBitmap( m_bmp );
		m_radio_C1.EnableWindow(1);
		m_radio_G1.EnableWindow(1);
		m_radio_Z1.EnableWindow(1);
		m_radio_C2.EnableWindow(1);
		m_radio_G2.EnableWindow(1);
		m_radio_Z2.EnableWindow(1);
		m_edit_C1.EnableWindow( m_radio_C1.GetCheck() );
		m_edit_G1.EnableWindow( m_radio_G1.GetCheck() );
		m_edit_Z1.EnableWindow( m_radio_Z1.GetCheck() );
		m_edit_C2.EnableWindow( m_radio_C2.GetCheck() );
		m_edit_G2.EnableWindow( m_radio_G2.GetCheck() );
		m_edit_Z2.EnableWindow( m_radio_Z2.GetCheck() );
		m_edit_hd.EnableWindow(1);
		m_edit_hpins.EnableWindow(1);
		m_combo_pin1.EnableWindow(1);
	}
	else if( m_type == DIP )
	{
		m_bmp.DeleteObject();
		m_bmp.LoadBitmap( IDB_BITMAP_DIP );
		m_bmp_quad.SetBitmap( m_bmp );
		m_radio_C1.EnableWindow(1);
		m_radio_G1.EnableWindow(1);
		m_radio_Z1.EnableWindow(1);
		m_radio_C2.EnableWindow(0);
		m_radio_G2.EnableWindow(0);
		m_radio_Z2.EnableWindow(0);
		m_edit_C1.EnableWindow( m_radio_C1.GetCheck() );
		m_edit_G1.EnableWindow( m_radio_G1.GetCheck() );
		m_edit_Z1.EnableWindow( m_radio_Z1.GetCheck() );
		m_edit_C2.EnableWindow(0);
		m_edit_G2.EnableWindow(0);
		m_edit_Z2.EnableWindow(0);
		m_edit_hd.EnableWindow(1);
		m_edit_hpins.EnableWindow(0);
		m_hpins = m_npins/2;
		m_vpins = 0;
		str.Format( "%d", m_hpins );
		m_edit_hpins.SetWindowText( str );
		m_edit_vpins.SetWindowText( "0" );
		m_combo_pin1.EnableWindow(0);
		m_combo_pin1.SetCurSel( 1 );
	}
	else if( m_type == HDR1 || m_type == HDR2 || m_type == BGA  )
	{
		m_bmp.DeleteObject();
		if( m_type == HDR1 )
			m_bmp.LoadBitmap( IDB_BITMAP_HDR1 );
		else if( m_type == HDR2 )
			m_bmp.LoadBitmap( IDB_BITMAP_HDR2 );
		else if( m_type == BGA )
			m_bmp.LoadBitmap( IDB_BITMAP_BGA );
		else
			ASSERT(0);
		m_bmp_quad.SetBitmap( m_bmp );
		m_radio_C1.EnableWindow(0);
		m_radio_G1.EnableWindow(0);
		m_radio_Z1.EnableWindow(0);
		m_radio_C2.EnableWindow(0);
		m_radio_G2.EnableWindow(0);
		m_radio_Z2.EnableWindow(0);
		m_edit_C1.EnableWindow(0);
		m_edit_G1.EnableWindow(0);
		m_edit_Z1.EnableWindow(0);
		m_edit_C2.EnableWindow(0);
		m_edit_G2.EnableWindow(0);
		m_edit_Z2.EnableWindow(0);
		m_edit_hd.EnableWindow(1);
		m_edit_hpins.EnableWindow(1);
		m_hpins = m_npins/2;
		m_vpins = 0;
		str.Format( "%d", m_hpins );
		m_edit_hpins.SetWindowText( str );
		m_edit_vpins.SetWindowText( "0" );
		m_combo_pin1.EnableWindow(0);
		if( m_type == BGA )
			m_combo_pin1.SetCurSel( 2 );
	}
	else if( m_type == EDGE || m_type == EDGE2 )
	{
		m_bmp.DeleteObject();
		if( m_type == EDGE )
			m_bmp.LoadBitmap( IDB_BITMAP_EDGE );
		else if( m_type == EDGE2 )
			m_bmp.LoadBitmap( IDB_BITMAP_EDGE2 );
		else
			ASSERT(0);
		m_bmp_quad.SetBitmap( m_bmp );
		m_combo_shape.SetCurSel( OVAL );
		m_shape = OVAL;
		m_radio_C1.EnableWindow(0);
		m_radio_G1.EnableWindow(0);
		m_radio_Z1.EnableWindow(0);
		m_radio_C2.EnableWindow(0); 
		m_radio_G2.EnableWindow(0);
		m_radio_Z2.EnableWindow(0);
		m_edit_C1.EnableWindow(0);
		m_edit_G1.EnableWindow(0);
		m_edit_Z1.EnableWindow(0);
		m_edit_C2.EnableWindow(0);
		m_edit_G2.EnableWindow(0);
		m_edit_Z2.EnableWindow(0);
		m_edit_hd.EnableWindow(0);
		m_edit_hd.SetWindowText("");
		m_edit_hpins.EnableWindow(0);
		m_hpins = m_npins/2;
		m_vpins = 2;
		m_edit_vpins.EnableWindow(0);
		m_edit_vpins.SetWindowText( "2" );
		str.Format( "%d", m_hpins );
		m_edit_hpins.SetWindowText( str );
		m_combo_pin1.EnableWindow(0);
		if( m_type == BGA )
			m_combo_pin1.SetCurSel( 2 );
	}
	else if( m_type == SIP )
	{
		m_bmp.DeleteObject();
		m_bmp.LoadBitmap( IDB_BITMAP_SIP );
		m_bmp_quad.SetBitmap( m_bmp );
		m_radio_C1.EnableWindow(0);
		m_radio_G1.EnableWindow(0);
		m_radio_Z1.EnableWindow(0);
		m_radio_C2.EnableWindow(0);
		m_radio_G2.EnableWindow(0);
		m_radio_Z2.EnableWindow(0);
		m_edit_C1.EnableWindow(0);
		m_edit_G1.EnableWindow(0);
		m_edit_Z1.EnableWindow(0);
		m_edit_C2.EnableWindow(0);
		m_edit_G2.EnableWindow(0);
		m_edit_Z2.EnableWindow(0);
		m_edit_hd.EnableWindow(1);
		m_edit_hpins.EnableWindow(0);
		m_hpins = m_npins;
		m_vpins = 0;
		str.Format( "%d", m_hpins );
		m_edit_hpins.SetWindowText( str );
		m_edit_vpins.SetWindowText( "0" );
		m_combo_pin1.EnableWindow(0);
		m_combo_pin1.SetCurSel( 1 );
	}
	OnEnChangeEditWizHpins();
}

void CDlgWizQuad::OnCbnSelchangeComboWizUnits()
{
	CString str;

	int test = m_combo_units.GetCurSel();
	if( test == 0 )
	{
		m_units = MIL;
		m_mult = 25400;
	}
	else
	{
		m_units = MM;
		m_mult = 1000000;
	}
	// reset all dimensions
	// CPT2 TODO.  I don't like this automatic conversion business.  When I start entering numbers, then belatedly realize I have mils where I
	// really want mm, it's annoying that then, because of the auto-conversion, I have to type all the numbers again.
	FormatDimensionString( &str, m_hd, m_units );
	m_edit_hd.SetWindowText( str );
	FormatDimensionString( &str, m_x, m_units );
	m_edit_x.SetWindowText( str );
	FormatDimensionString( &str, m_y, m_units );
	m_edit_y.SetWindowText( str );
	FormatDimensionString( &str, m_r, m_units );
	m_edit_radius.SetWindowText( str );
	FormatDimensionString( &str, m_e, m_units );
	m_edit_E.SetWindowText( str );
	FormatDimensionString( &str, m_c1, m_units );
	m_edit_C1.SetWindowText( str );
	FormatDimensionString( &str, m_g1, m_units );
	m_edit_G1.SetWindowText( str );
	FormatDimensionString( &str, m_z1, m_units );
	m_edit_Z1.SetWindowText( str );
	FormatDimensionString( &str, m_c2, m_units );
	m_edit_C2.SetWindowText( str );
	FormatDimensionString( &str, m_g2, m_units );
	m_edit_G2.SetWindowText( str );
	FormatDimensionString( &str, m_z2, m_units );
	m_edit_Z2.SetWindowText( str );
}

void CDlgWizQuad::FormatDimensionString( CString * str, int dim, int units )
{
	if( units == MIL )
		str->Format( "%d", dim/NM_PER_MIL );
	else if( units == MM )
		str->Format( "%6.3f", (double)dim/1000000.0 );
	else
		ASSERT(0);

}

// check parameters, make footprint and call Save dialog
//
void CDlgWizQuad::OnBnClickedButtonSave()
{
	// try to make the footprint
	if( !MakeFootprint() )
		return;

	if( m_enable_save )
	{
		// if saving is enabled, do it
		CDlgSaveFootprint dlg;
		dlg.Initialize( &m_str_name, m_footprint, m_units, "", 
			m_footprint_cache_map, m_footlibfoldermap, m_dlg_log );	
		int test = dlg.DoModal();
	}
	else
		// otherwise, we are done
		OnOK();
}

void CDlgWizQuad::OnCbnSelchangeComboPin1()
{
	m_pin1 = m_combo_pin1.GetCurSel();
}

void CDlgWizQuad::OnEnChangeEditWizHd()
{
	CString str;
	m_edit_hd.GetWindowText( str );
	m_hd = atof( str ) * m_mult;
}

void CDlgWizQuad::OnBnClickedWizButtonExit()
{
	OnCancel();
}

void CDlgWizQuad::OnBnClickedButtonPreview()
{
	if( !MakeFootprint() )
		return;

	// draw preview of footprint
	CMetaFileDC m_mfDC;
	CDC * pDC = this->GetDC();
	CRect rw;
	m_preview.GetClientRect( &rw );
	HENHMETAFILE hMF = m_footprint->CreateMetafile( &m_mfDC, pDC, rw );
	m_preview.SetEnhMetaFile( hMF );
	ReleaseDC( pDC );
	DeleteEnhMetaFile( hMF );
}

BOOL CDlgWizQuad::MakeFootprint()
{
	CString str_name;
	CString str;
	CString str_E;
	CString str_hd;
	CString str_X;
	CString str_Y;
	CString str_R;
	CString str_C1;
	CString str_C2;
	CString str_RL;
	CString str_RB;

	OnCbnSelchangeComboPin1();

	// create footprint and call Save dialog.
	m_footprint->Clear();
	m_footprint->m_name = "";

	// first check for legal parameters
	m_edit_name.GetWindowText( m_str_name );
	m_str_name.Trim();
	if( m_str_name.GetLength() == 0 )
	{
		CString s ((LPCSTR) IDS_IllegalName);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_npins < 1 )
	{
		CString s ((LPCSTR) IDS_IllegalNumberOfPins);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_npins > MAX_PINS )
	{
		CString s ((LPCSTR) IDS_TooManyPins);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_type == HDR1 || m_type == HDR2 || m_type == BGA )
	{
		if( m_npins != m_hpins*m_vpins )
		{
			CString s ((LPCSTR) IDS_IllegalNumberOfPins);
			AfxMessageBox( s );
			return FALSE;
		}
	}
	if( m_hpins < 1 || m_hpins > m_npins )
	{
		CString s ((LPCSTR) IDS_IllegalNumberOfPinsPerHorizontalRow);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_vpins < 0 || m_vpins > m_npins )
	{
		CString s ((LPCSTR) IDS_IllegalNumberOfPinsPerVerticalRow);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_vpins > MAX_BGA_ROWS && m_type == BGA )
	{
		CString s ((LPCSTR) IDS_TooManyRows);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_x <= 0 && m_shape != NONE ) 
	{
		CString s ((LPCSTR) IDS_IllegalPadWidthX);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_y <= 0  && (m_shape == RECT || m_shape == nliECT || m_shape == OVAL ) )
	{
		CString s ((LPCSTR) IDS_IllegalPadLengthY);
		AfxMessageBox( s ); 
		return FALSE;
	}
	if( m_r <= 0  && m_shape == nliECT )
	{
		CString s ((LPCSTR) IDS_IllegalPadCornerRadius);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_r > 0.5*min(m_y,m_x) && m_shape == nliECT )
	{
		CString s ((LPCSTR) IDS_PadCornerRadiusTooLarge);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_shape == NONE && m_hd == 0 )
	{
		CString s ((LPCSTR) IDS_PadShapeNoneRequiresANonZeroHoleWidth);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_e <= 0 && m_hpins != 1 )
	{
		CString s ((LPCSTR) IDS_IllegalPadSpacingE);
		AfxMessageBox( s );
		return FALSE;
	}
	if( (m_type == QUAD || m_type == DIP) && m_g1 <= 0 )
	{
		CString s ((LPCSTR) IDS_IllegalDistanceBetweenTopAndBottomRows);
		AfxMessageBox( s );
		return FALSE;
	}
	if( m_type == QUAD && m_g2 <= 0 )
	{
		CString s ((LPCSTR) IDS_IllegalDistanceBetweenLeftAndRightRows);
		AfxMessageBox( s );
		return FALSE;
	}

	if( m_type == EDGE || m_type == EDGE2 )
	{
		// get pad info
		int pad_type;
		if( m_shape == RECT )
			pad_type = PAD_RECT;
		else if( m_shape == nliECT )
			pad_type = PAD_RRECT;
		else if( m_shape == OVAL )
			pad_type = PAD_OVAL;
		else
		{
			CString s ((LPCSTR) IDS_IllegalPadShapeForEdgeConnector);
			AfxMessageBox( s );
			return FALSE;
		}

		// make footprint directly
		m_footprint->Clear();
		m_footprint->m_name = m_str_name; 
		m_footprint->m_units = m_units;
		m_footprint->m_ref->m_font_size = 50*NM_PER_MIL; 
		m_footprint->m_ref->m_x = 0; 
		m_footprint->m_ref->m_y = m_y/2 + 10*NM_PER_MIL; 
		m_footprint->m_ref->m_angle = 0;	
		m_footprint->m_ref->m_stroke_width = 7*NM_PER_MIL;
		m_footprint->m_sel_xi = -m_x;
		m_footprint->m_sel_yi = -m_y/2 - 10*NM_PER_MIL;
		m_footprint->m_sel_xf = m_e * (m_hpins-1) + m_x; 
		m_footprint->m_sel_yf = m_y/2 + 10*NM_PER_MIL;
		for( int i=0; i<m_npins; i++ )
		{
			cpadstack * ps = new cpadstack (m_footprint->m_doc);
			ps->hole_size = 0;
			ps->angle = 90;
			ps->y_rel = 0;
			ps->top.shape = PAD_NONE;
			ps->inner.shape = PAD_NONE;
			ps->bottom.shape = PAD_NONE;
			CString pin_name;
			pin_name.Format( "%d", i+1 );
			ps->name = pin_name;
			ps->x_rel = i*m_e;
			if( i>=m_npins/2 )
			{
				if( m_type == EDGE )
					ps->x_rel = (i - m_npins/2)*m_e;
				else
					ps->x_rel = (m_npins - i - 1)*m_e;
				ps->bottom.shape = pad_type;
				ps->bottom.size_h = m_x;
				ps->bottom.size_l = m_y/2;
				ps->bottom.size_r = m_y/2;
				ps->bottom.radius = m_r;
			}
			else
			{
				ps->top.shape = pad_type;
				ps->top.size_h = m_x;
				ps->top.size_l = m_y/2;
				ps->top.size_r = m_y/2;
				ps->top.radius = m_r;
			}
		}
	}
	else
	{
		// make footprint definition string
		MakeCStringFromDimension( &str_hd, m_hd, m_units );
		MakeCStringFromDimension( &str_X, m_x, m_units );
		MakeCStringFromDimension( &str_Y, m_y/2, m_units );
		MakeCStringFromDimension( &str_R, m_r, m_units );
		MakeCStringFromDimension( &str_E, m_e, m_units );
		if( m_type == QUAD )
		{
			// make string like "QFP_44_H11_P1TC_PH50_RL50_RB50_RECT_PW24_PE24_PI14"
			MakeCStringFromDimension( &str_C1, m_c1, m_units );
			MakeCStringFromDimension( &str_C2, m_c2, m_units );
			int rl = (m_c2 - m_e*(m_hpins-1))/2;
			MakeCStringFromDimension( &str_RL, rl, m_units );
			int rb = (m_c1 - m_e*(m_vpins-1))/2;
			MakeCStringFromDimension( &str_RB, rb, m_units );
			str.Format("QFP_%d_H%d_", m_npins, m_hpins );
			if( m_pin1 == TOP_CENTER )
				str += "P1TC_";
			else if( m_pin1 == BOTTOM_LEFT )
				str += "P1BL_";
			else if( m_pin1 == TOP_LEFT ) 
				str += "P1TL_";
			else 
				ASSERT(0);
			str += "RL" + str_RL + "_";
			str += "RB" + str_RB + "_";
		}
		else if( m_type == HDR1 )
		{
			// make string like "HDR_6_H3_BTLR_PV100_PH100_RECT_PW50_PE40_PI40_HD24"
			str.Format( "HDR_%d_H%d_BTLR_PV%s_", m_npins, m_hpins, str_E );
		}
		else if( m_type == HDR2 || m_type == BGA )
		{
			// make string like "HDR_6_H3_LRBT_PV100_PH100_RECT_PW50_PE40_PI40_HD24"
			str.Format( "HDR_%d_H%d_LRBT_PV%s_", m_npins, m_hpins, str_E );
		}
		else if( m_type == DIP )
		{
			// make string like "DIP_20_H10_RT300_PH100_RECT_PW50_PE40_PI40_HD24"
			MakeCStringFromDimension( &str_C1, m_c1, m_units );
			str.Format("DIP_%d_H%d_RT%s_", m_npins, m_hpins, str_C1 );
		}
		else if( m_type == SIP )
		{
			// make string like "HDR_10_H10_PH100_RECT_PW24_PE24_PI14_HD24"
			str.Format("SIP_%d_H%d_", m_npins, m_hpins );
		}

		// set units for output
		if( m_units == MM )
			str += "UMM_";
		else if( m_units == MIL )
			str += "UMIL_";

		// set pad shape and dimensions
		if( m_shape == RECT ) 
			str += "RECT_";
		else if( m_shape == SQ1 )
			str += "RNDSQ1_";
		else if( m_shape == SQUARE )
			str += "SQUARE_";
		else if( m_shape == ROUND )
			str += "ROUND_";
		else if( m_shape == OCTAGON )
			str += "OCTAGON_";
		else if( m_shape == nliECT )
			str += "RNDRECT_";
		else if( m_shape == OVAL )
			str += "OVAL_";
		else if( m_shape == NONE )
			str += "NONE_";
		else
			ASSERT(0);
		str += "PH" + str_E + "_";
		str += "PW" + str_X + "_";
		if( m_shape == RECT || m_shape == OVAL || m_shape == nliECT )
			str += "PE" + str_Y + "_PI" + str_Y + "_";
		if( m_shape == nliECT )
			str += "PR" + str_R + "_";
		str += "HW" + str_hd;

		// OK, go for it
		int err = m_footprint->MakeFromString( m_str_name, str );
		if( err )
		{
			CString err_str, s ((LPCSTR) IDS_CShapeMakeFromStringFailed);
			err_str.Format(s, str); 
			AfxMessageBox( err_str );
			m_footprint->Clear();
			return FALSE;
		}
		// CPT2.  Note that MakeFromString() set the utility value for each pin in m_footprint, equal to that pin's (0-based) ordinal number
		if ( m_type == BGA )
		{
			// rename pins and modify pad shape for SQ1 patterns
			int ip_A1 = (m_vpins-1)*m_hpins;
			citer<cpadstack> ips (&m_footprint->m_padstack);
			for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
			{
				int iv = m_vpins - ps->utility/m_hpins - 1;
				int ih = ps->utility % m_hpins;
				ps->name.Format( "%s%d", &bga_row_name[iv][0], ih+1 );
				if (m_shape == SQ1)
					if (ps->utility==0)
						ps->top.shape = ps->inner.shape = ps->bottom.shape = PAD_ROUND;
					else if (ps->utility==ip_A1)
						ps->top.shape = ps->bottom.shape = PAD_SQUARE,
						ps->inner.shape = PAD_ROUND;
			}
			// modify outline.  CPT2 an ugly business...
			int chamfer = m_e;
			int enlarge = m_e/2;
			ccontour *ctr = m_footprint->m_outline_poly.First()->main;
			ccorner *c = ctr->head;
			c->x -= enlarge;					// Bottom left
			c->y -= enlarge;
			c = c->postSide->postCorner;
			c->x += enlarge;					// Bottom right
			c->y -= enlarge;
			c = c->postSide->postCorner;
			c->x += enlarge;					// Top right
			c->y += enlarge;
			c = c->postSide->postCorner;
			int tlx = c->x, tly = c->y;			// Top left (add an extra corner for chamfering)
			c->x = tlx+chamfer-enlarge;
			c->y = tly+enlarge;
			c->postSide->InsertCorner( tlx-enlarge, tly-chamfer+enlarge );
			m_footprint->m_sel_xi -= enlarge;
			m_footprint->m_sel_yi -= enlarge;
			m_footprint->m_sel_xf += enlarge;
			m_footprint->m_sel_yf += enlarge;
		}
	}

	// Yay!
	return TRUE;
}
