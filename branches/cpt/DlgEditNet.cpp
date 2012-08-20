// DlgEditNet.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgEditNet.h"
#include ".\dlgeditnet.h"
#include "PartListNew.h"
#include "NetListNew.h"


// CDlgEditNet dialog

IMPLEMENT_DYNAMIC(CDlgEditNet, CDialog)
CDlgEditNet::CDlgEditNet(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgEditNet::IDD, pParent)
{
	m_pins_edited = FALSE;
}

CDlgEditNet::~CDlgEditNet()
{
}

void CDlgEditNet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_DEF_WIDTH, m_button_def_width);
	DDX_Control(pDX, IDC_RADIO_SET_WIDTH, m_button_set_width);
	DDX_Control(pDX, IDC_EDIT_VIA_PAD_W, m_edit_pad_w);
	DDX_Control(pDX, IDC_EDIT_VIA_HOLE_W, m_edit_hole_w);
	DDX_Control(pDX, IDC_LIST_PIN, m_list_pins);
	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Control(pDX, IDC_CHECK_VISIBLE, m_check_visible);
	DDX_Control(pDX, IDC_COMBO_WIDTH, m_combo_width);
	DDX_Control(pDX, IDC_EDIT_ADD_PIN, m_edit_addpin);
	DDX_Control(pDX, IDC_CHECK1, m_check_apply);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add_pin);
	DDX_Control(pDX, ID_MY_OK, m_button_OK);
	if( pDX->m_bSaveAndValidate )
	{
		// now implement edits into netlist_info
		m_edit_name.GetWindowText( m_name );
		if( m_name.GetLength() > MAX_NET_NAME_SIZE )
		{
			CString mess, s ((LPCSTR) IDS_MaxLengthOfNetName);
			mess.Format( s, MAX_NET_NAME_SIZE ); 
			AfxMessageBox( mess );
			pDX->Fail();
		}

		int i;
		DDX_Text( pDX, IDC_COMBO_WIDTH, i );
		if( i<0 )
		{
			CString s ((LPCSTR) IDS_IllegalTraceWidth);
			AfxMessageBox( s );
			pDX->Fail();
		}
		m_def_w = i*NM_PER_MIL; 

		DDX_Text( pDX, IDC_EDIT_VIA_PAD_W, i );
		if( i<0 )
		{
			CString s ((LPCSTR) IDS_IllegalViaWidth);
			AfxMessageBox( s );
			pDX->Fail();
		}
		m_def_v_w = i*NM_PER_MIL; 

		DDX_Text( pDX, IDC_EDIT_VIA_HOLE_W, i );
		if( i<0 )
		{
			CString s ((LPCSTR) IDS_IllegalViaHoleWidth);
			AfxMessageBox( s );
			pDX->Fail();
		}
		m_def_v_h_w = i*NM_PER_MIL; 
		m_visible = m_check_visible.GetState();
		for( int in=0; in<m_nli->GetSize(); in++ )
		{
			if( in != m_in && m_name == (*m_nli)[in].name && !(*m_nli)[in].deleted )
			{
				CString s ((LPCSTR) IDS_DuplicateNetName);
				AfxMessageBox( s );
				pDX->Fail();
			}
		}

		// now update netlist_info
		if( m_new_net )
		{
			// add entry to netlist_info for new net, set index m_in
			int num_nets = m_nli->GetSize();
			m_nli->SetSize( num_nets + 1 );
			m_in = num_nets;
			(*m_nli)[m_in].net = NULL;
		}
		net_info *ni = &(*m_nli)[m_in];
		ni->apply_trace_width = m_check_apply.GetCheck();
		ni->apply_via_width = m_check_apply.GetCheck();
		ni->modified = TRUE;
		ni->name = m_name;
		ni->visible = m_visible;
		ni->w = m_def_w;
		ni->v_w = m_def_v_w;
		ni->v_h_w = m_def_v_h_w;
		if( m_pins_edited )
		{
			int npins = m_list_pins.GetCount();
			ni->ref_des.SetSize( npins );
			ni->pin_name.SetSize( npins );
			for( int i=0; i<npins; i++ )
			{
				// now parse pin string from listbox
				CString str;
				m_list_pins.GetText( i, str );
				str.Trim();
				int len = str.GetLength();
				if( len <= 3 )
					ASSERT(0);
				int dot_pos = str.FindOneOf( "." );
				if( dot_pos < 2 || dot_pos >= (len-1) )
					ASSERT(0);
				CString refstr = str.Left(dot_pos);
				CString pinstr = str.Right( len - dot_pos - 1 );
				int pin = atoi( pinstr );
				ni->ref_des[i] = refstr;
				ni->pin_name[i] = pinstr;
				// now remove pin from other net, if necessary
				for( int in=0; in<m_nli->GetSize(); in++ )
				{
					net_info *ni = &(*m_nli)[in];
					if( in != m_in && !ni->deleted )
						for( int ip = 0; ip < ni->ref_des.GetSize(); ip++ )
							if( refstr == ni->ref_des[ip] && pinstr == ni->pin_name[ip] )
							{
								ni->modified = TRUE;
								ni->ref_des.RemoveAt( ip );
								ni->pin_name.RemoveAt( ip );
								break;
							}
				}
			}
		}
	}
	else
	{
		// convert units
		m_def_w = m_def_w/NM_PER_MIL;
		m_def_v_w = m_def_v_w/NM_PER_MIL;
		m_def_v_h_w = m_def_v_h_w/NM_PER_MIL;
		// default is to apply new trace width.
		// CPT2 I changed this.  If you want to just rename a net or the like, it's all too easy to mistakenly change all 
		// the trace widths this way...
		m_check_apply.SetCheck(0);
	}
}

void CDlgEditNet::Initialize( netlist_info * nli,
							 int i,
							 cpartlist * plist,
							 BOOL new_net,
							 BOOL visible,
							 int units,
							 CArray<int> * w,
							 CArray<int> * v_w,
							 CArray<int> * v_h_w )
{
	m_units = units;
	m_nli = nli;
	m_in = i;
	m_plist = plist;
	m_visible = visible;
	m_new_net = new_net;
	if( new_net )
	{
		m_name = "";
		m_def_w = 0;
		m_def_v_w = 0;
		m_def_v_h_w = 0;
	}
	else
	{
		m_name = (*nli)[i].name;
		m_def_w = (*nli)[i].w;
		m_def_v_w = (*nli)[i].v_w;
		m_def_v_h_w = (*nli)[i].v_h_w;
	}
	m_w = w;
	m_v_w = v_w;
	m_v_h_w = v_h_w;
}

BEGIN_MESSAGE_MAP(CDlgEditNet, CDialog)
	ON_BN_CLICKED(IDC_RADIO_DEF_WIDTH, OnBnClickedRadioDefWidth)
	ON_BN_CLICKED(IDC_RADIO_SET_WIDTH, OnBnClickedRadioSetWidth)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH, OnCbnSelchangeComboWidth)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchangeComboWidth)
//	ON_WM_PAINT()
//	ON_WM_NCPAINT()
ON_EN_UPDATE(IDC_EDIT_ADD_PIN, OnEnUpdateEditAddPin)
ON_BN_CLICKED(ID_MY_OK, OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgEditNet message handlers

void CDlgEditNet::OnBnClickedRadioDefWidth()
{
	m_edit_pad_w.EnableWindow( 0 );
	m_edit_hole_w.EnableWindow( 0 );
}

void CDlgEditNet::OnBnClickedRadioSetWidth()
{
	m_edit_pad_w.EnableWindow( 1 );
	m_edit_hole_w.EnableWindow( 1 );
}

void CDlgEditNet::OnBnClickedButtonDelete()
{
	// Get the indexes of all the selected pins and delete them
	int nCount = m_list_pins.GetSelCount();
	CArray<int,int> aryListBoxSel;
	aryListBoxSel.SetSize(nCount);
	m_list_pins.GetSelItems(nCount, aryListBoxSel.GetData()); 
	for( int j=(nCount-1); j>=0; j-- )
	{
		int iItem = aryListBoxSel[j];
		m_list_pins.DeleteString( iItem );
	}
	m_pins_edited = TRUE;
}

void CDlgEditNet::OnBnClickedButtonAdd()
{
	CString str;

	m_edit_addpin.GetWindowText( str );
	str.Trim();
	int len = str.GetLength();
	if( len < 3 )
	{
		CString s ((LPCSTR) IDS_IllegalPin);
		AfxMessageBox( s );
		return;
	}
	else
	{
		int test = m_list_pins.FindString( 0, str );
		if( test != -1 )
		{
			CString s ((LPCSTR) IDS_PinAlreadyInThisNet);
			AfxMessageBox( s );
			return;
		}
		int dot_pos = str.FindOneOf( "." );
		if( dot_pos >= 2 && dot_pos < (len-1) )
		{
			CString refstr = str.Left( dot_pos );
			cpart2 * part = m_plist->GetPartByName( &refstr );
			if( !part )
			{
				CString s ((LPCSTR) IDS_PartDoesNotExist);
				str.Format( s, refstr );
				AfxMessageBox( str );
				return;
			}
			if( !part->shape )
			{
				CString s ((LPCSTR) IDS_PartDoesNotHaveAFootprint);
				str.Format( s, refstr );
				AfxMessageBox( str );
				return;
			}
			CString pinstr = str.Right( len - dot_pos - 1 );
			if( !CheckLegalPinName( &pinstr ) )
			{
				CString s1 ((LPCSTR) IDS_PinNameMayNotContainAnyOfTheCharacters);
				AfxMessageBox( s1 );
				return;
			}
			cpin2 *pin = part->GetPinByName( &pinstr );
			if( !pin )
			{
				CString s ((LPCSTR) IDS_PinNotFoundInFootprint);
				str.Format( s, pinstr, part->shape->m_name );
				AfxMessageBox( str );
				return;
			}
			// now search for pin in m_nli
			int i_found = -1;
			for( int i=0; i<m_nli->GetSize(); i++ )
			{
				net_info *ni = &(*m_nli)[i];
				if( ni->deleted )
					continue;
				for( int ip = 0; ip < ni->ref_des.GetSize(); ip++ )
					if( refstr == ni->ref_des[ip] && pinstr == ni->pin_name[ip] )
						{ i_found = i; break; }
			}
			if( i_found != -1 )
			{
				if( i_found == m_in )
				{
					CString s ((LPCSTR) IDS_PinAlreadyInThisNet);
					AfxMessageBox( s );
					return;
				}
				else
				{
					CString mess, s ((LPCSTR) IDS_PinIsAssignedToNet);
					mess.Format( s, refstr, pinstr, (*m_nli)[i_found].name, (*m_nli)[i_found].name );
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret != IDOK )
						return;
				}
			}
			m_list_pins.AddString( refstr + "." + pinstr );
			m_pins_edited = TRUE;
			m_button_OK.SetButtonStyle( BS_DEFPUSHBUTTON );
			m_button_add_pin.SetButtonStyle( BS_PUSHBUTTON );
			m_edit_addpin.SetWindowText( "" );
		}
	}
}

// enter with the following variables set up by calling function:
//	m_new_net = TRUE to add net, FALSE to edit net
//	m_name = pointer to CString with net name (or "" if adding new net)
//  m_use_nl_not_nlist = TRUE to use m_nli, FALSE to use m_nlist for net info
//	m_nli = pointer to net_info array 
//	m_in = index into m_nli for this net
//	m_plist = pointer to partlist
//	m_w = pointer to CArray of trace widths
//	m_v_w = pointer to CArray of via pad widths
//	m_v_h_w = pointer to CArray of via hole widths
//	m_visible = visibility flag
//
BOOL CDlgEditNet::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString str;
	m_edit_name.SetWindowText( m_name );
	str.Format( "%d", m_def_w );
	m_combo_width.SetWindowText( str );
	str.Format( "%d", m_def_v_w );
	m_edit_pad_w.SetWindowText( str );
	str.Format( "%d", m_def_v_h_w );
	m_edit_hole_w.SetWindowText( str );
	m_edit_pad_w.EnableWindow( 0 );
	m_edit_hole_w.EnableWindow( 0 );
	m_button_def_width.SetCheck( 1 );
	m_button_set_width.SetCheck( 0 );
	m_check_visible.SetCheck( m_visible );
	// set up combo box for trace widths
	int n = m_w->GetSize();
	for( int i=0; i<n; i++ )
	{
		str.Format( "%d", (*m_w)[i]/NM_PER_MIL );
		m_combo_width.InsertString( i, str );
	}
	// set up list box for pins
	if( !m_new_net )
	{
		int npins = (*m_nli)[m_in].ref_des.GetSize();
		for( int i=0; i<npins; i++ )
		{
			str.Format( "%s.%s", (*m_nli)[m_in].ref_des[i], (*m_nli)[m_in].pin_name[i] ); 
			m_list_pins.InsertString( i, str );
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgEditNet::OnCbnSelchangeComboWidth()
{
	CString test;
	int i = m_combo_width.GetCurSel();
	m_combo_width.GetLBText( i, test );
	ChangeTraceWidth( test );
}

void CDlgEditNet::OnCbnEditchangeComboWidth()
{
	CString test;
	m_combo_width.GetWindowText( test );
	ChangeTraceWidth( test );
}


void CDlgEditNet::ChangeTraceWidth( CString test )
{
	int n = m_w->GetSize();
	if( m_button_def_width.GetCheck() )
	{
		int new_w = atoi( (LPCSTR)test )*NM_PER_MIL;
		int new_v_w = 0;
		int new_v_h_w = 0;
		if( new_w >= 0 )
		{
			if( new_w == 0 )
			{
				new_v_w = 0;
				new_v_h_w = 0;
			}
			else if( new_w <= (*m_w)[0] )
			{
				new_v_w = (*m_v_w)[0];
				new_v_h_w = (*m_v_h_w)[0];
			}
			else if( new_w >= (*m_w)[n-1] )
			{
				new_v_w = (*m_v_w)[n-1];
				new_v_h_w = (*m_v_h_w)[n-1];
			}
			else
			{
				for( int i=1; i<n; i++ )
				{
					if( new_w > (*m_w)[i-1] && new_w <= (*m_w)[i] ) 
					{
						new_v_w = (*m_v_w)[i];
						new_v_h_w = (*m_v_h_w)[i];
						break;
					}
				}
			}
			test.Format( "%d", new_v_w/NM_PER_MIL );
			m_edit_pad_w.SetWindowText( test );
			test.Format( "%d", new_v_h_w/NM_PER_MIL );
			m_edit_hole_w.SetWindowText( test );
		}
	}
}

void CDlgEditNet::SetFields()
{
}

void CDlgEditNet::GetFields()
{
}

void CDlgEditNet::OnEnUpdateEditAddPin()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.
	m_button_OK.SetButtonStyle( BS_PUSHBUTTON );
	m_button_add_pin.SetButtonStyle( BS_DEFPUSHBUTTON );
}

void CDlgEditNet::OnBnClickedOk()
{
	// if we are adding pins, trap "Enter" key
	void * focus_ptr = this->GetFocus();
	void * addpin_ptr = &m_edit_addpin;
	CString str;
	m_edit_addpin.GetWindowText( str );
	if( focus_ptr == addpin_ptr && str.GetLength() > 0 )
		OnBnClickedButtonAdd();
	else
		OnOK();
}
