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
	m_nlist = 0;
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
		if( m_nlist )
		{
			CIter<CNet> in (&m_nlist->nets);
			for (CNet *net = in.First(); net; net = in.Next())	
				m_combo_net.AddString( net->name );
		}
		::ShowCursor( TRUE );	// force cursor
	}
	else
	{
		// outgoing
		pDX->PrepareCtrl( IDC_COMBO_NET );
		if( m_net_str == "" )
		{
			CString s ((LPCSTR) IDS_IllegalNetName);
			AfxMessageBox( s );
			pDX->Fail();
		}
		if (m_net_str!=created_name && !m_nlist->GetNetPtrByName(&m_net_str))
		{
			CString s ((LPCSTR) IDS_NetNotFoundInNetlist), str;
			str.Format(s, m_net_str);
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
		if (!m_nlist->GetNetPtrByName(&str))
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
