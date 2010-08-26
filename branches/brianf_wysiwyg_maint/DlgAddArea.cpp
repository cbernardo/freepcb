// DlgAddArea.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddArea.h"
#include "layers.h"

// globals
int gHatch = CPolyLine::NO_HATCH;

// CDlgAddArea dialog

IMPLEMENT_DYNAMIC(CDlgAddArea, CDialog)
CDlgAddArea::CDlgAddArea(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddArea::IDD, pParent)
{
}

CDlgAddArea::~CDlgAddArea()
{
}

BOOL CDlgAddArea::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_slider_opacity.SetRange(0,100);
	m_slider_opacity.SetTicFreq(10);

	return TRUE; // return TRUE unless you set the focus to a control
}

void CDlgAddArea::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_NET,  m_combo_net);
	DDX_Control(pDX, IDC_LIST_LAYER, m_list_layer);
	DDX_Control(pDX, IDC_RADIO_NONE, m_radio_none);
	DDX_Control(pDX, IDC_RADIO_FULL, m_radio_full);
	DDX_Control(pDX, IDC_RADIO_EDGE, m_radio_edge);
	DDX_Control(pDX, IDC_SLIDER1,    m_slider_opacity);

	if( !pDX->m_bSaveAndValidate )
	{
		// incoming, initialize net list
		CIterator_cnet iter(m_nlist);
		for( cnet * net = iter.GetFirst(); net != NULL; net = iter.GetNext() )
		{
			m_combo_net.AddString( net->name );
		}

		if( m_net )
		{
			bNewArea = FALSE;
			int i = m_combo_net.SelectString( -1, m_net->name );
			if( i == CB_ERR )
				ASSERT(0);
		}
		else
		{
			bNewArea = TRUE;
		}

		// initialize layers
		m_num_layers = m_num_layers-LAY_TOP_COPPER;
		for( int il=0; il<m_num_layers; il++ )
		{
			m_list_layer.InsertString( il, &layer_str[il+LAY_TOP_COPPER][0] );
		}
		m_list_layer.SetCurSel( m_layer - LAY_TOP_COPPER );
		if( m_hatch == -1 )
			m_hatch = gHatch;	
		if( m_hatch == CPolyLine::NO_HATCH )
			m_radio_none.SetCheck( 1 );
		else if( m_hatch == CPolyLine::DIAGONAL_EDGE )
			m_radio_edge.SetCheck( 1 );
		else if( m_hatch == CPolyLine::DIAGONAL_FULL )
			m_radio_full.SetCheck( 1 );

		m_slider_opacity.SetPos(m_opacity);
	}
	else
	{
		// outgoing
		m_layer = m_list_layer.GetCurSel() + LAY_TOP_COPPER;
		m_combo_net.GetWindowText( m_net_name );

		POSITION pos;
		CString name;
		void * ptr;
		m_net = m_nlist->GetNetPtrByName( m_net_name );
		if( !m_net )
		{
			AfxMessageBox( "Illegal net name" );
			pDX->Fail();
		}
		if( m_radio_none.GetCheck() )
			m_hatch = CPolyLine::NO_HATCH;
		else if( m_radio_full.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_FULL;
		else if( m_radio_edge.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_EDGE;
		else 
			ASSERT(0);
		if( bNewArea )
			gHatch = m_hatch;

		m_opacity = m_slider_opacity.GetPos();
	}
}


BEGIN_MESSAGE_MAP(CDlgAddArea, CDialog)
END_MESSAGE_MAP()


// CDlgAddArea message handlers

void CDlgAddArea::Initialize( CNetList * nl, int nlayers, 
					 		  cnet * net, int layer, int hatch, int opacity )
{
	m_nlist = nl;
	m_num_layers = nlayers;
	m_net = net;
	m_layer = layer;
	m_hatch = hatch;
	m_opacity = opacity;
}
