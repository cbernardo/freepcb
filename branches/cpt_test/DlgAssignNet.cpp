// DlgAssignNet.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAssignNet.h"
#include ".\dlgassignnet.h"


// DlgAssignNet dialog

IMPLEMENT_DYNAMIC(DlgAssignNet, CDialog)
DlgAssignNet::DlgAssignNet(CWnd* pParent /*=NULL*/)
	: CDialog(DlgAssignNet::IDD, pParent)
{
	m_net_str = "";
	m_map = 0;
	created_name = "";
}

DlgAssignNet::~DlgAssignNet()
{
}

void DlgAssignNet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBString( pDX, IDC_COMBO_NET, m_net_str );
	DDV_MaxChars( pDX, m_net_str, MAX_NET_NAME_SIZE+1 );
	DDX_Control(pDX, IDC_COMBO_NET, m_combo_net);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		if( m_map )
		{
			POSITION pos;
			CString key;
			void * ptr;

			// Iterate through the entire netlist
			for( pos = m_map->GetStartPosition(); pos != NULL; )
			{
				m_map->GetNextAssoc( pos, key, ptr );
				m_combo_net.AddString( key );
			}
		}
		::ShowCursor( TRUE );	// force cursor
	}
	else
	{
		// outgoing
		pDX->PrepareCtrl( IDC_COMBO_NET );
		if( m_net_str == "" )
		{
			AfxMessageBox( "Illegal net name" );
			pDX->Fail();
		}
		void * ptr;
		if( m_net_str != created_name && !m_map->Lookup( m_net_str, ptr ) )
		{
			CString str = "Net \"" + m_net_str + "\" not found in netlist\nCreate it ?"; 
			int ret = AfxMessageBox( str, MB_YESNO );
			if( ret == IDNO )
			{
				pDX->Fail();
			}
		}
		::ShowCursor( FALSE );	// restore cursor
	}
}


BEGIN_MESSAGE_MAP(DlgAssignNet, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_NEW_NET, OnBnClickedButtonNewNet)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// DlgAssignNet message handlers

void DlgAssignNet::OnBnClickedButtonNewNet()
{
	CString str;
	int i = 0;
	BOOL bFound = TRUE;
	while( bFound )
	{
		i++;
		str.Format( "N%.5d", i );
		void * ptr;
		if( !m_map->Lookup( str, ptr ) )
			bFound = FALSE;
	}
	m_combo_net.SetWindowText( str );
	created_name = str;
}

void DlgAssignNet::OnBnClickedCancel()
{
	::ShowCursor( FALSE );	// restore cursor
	OnCancel();
}
