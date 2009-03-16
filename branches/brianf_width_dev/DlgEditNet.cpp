// DlgEditNet.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgEditNet.h"
#include ".\dlgeditnet.h"


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


void CDlgEditNet::Initialize( CNetList const * netlist,
							  netlist_info * nl,
							  int i,
							  CPartList * plist,
							  BOOL new_net,
							  BOOL visible,
							  int units,
							  CArray<int> * w,
							  CArray<int> * v_w,
							  CArray<int> * v_h_w )
{
	m_units = units;
	m_nl = nl;
	m_in = i;
	m_plist = plist;
	m_visible = visible;
	m_new_net = new_net;
	if( new_net )
	{
		m_name = "";

		// Set all attib to use parent
		m_width_attrib = CConnectionWidthInfo(
			CInheritableInfo::E_USE_PARENT,
			CInheritableInfo::E_USE_PARENT,
			CInheritableInfo::E_USE_PARENT
		);
		m_width_attrib = CClearanceInfo( CInheritableInfo::E_USE_PARENT );

		m_width_attrib.SetParent( netlist->Get_def_width_attrib() );
	}
	else
	{
		m_name         = (*nl)[i].name;
		m_width_attrib = (*nl)[i].width_attrib;
	}
	m_w = w;
	m_v_w = v_w;
	m_v_h_w = v_h_w;
}


// enter with the following variables set up by calling function:
//	m_new_net = TRUE to add net, FALSE to edit net
//	m_name = pointer to CString with net name (or "" if adding new net)
//  m_use_nl_not_nlist = TRUE to use m_nl, FALSE to use m_nlist for net info
//	m_nl = pointer to net_info array
//	m_in = index into m_nl for this net
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

	m_width_attrib.Update();

	m_edit_name.SetWindowText( m_name );
	m_check_visible.SetCheck( m_visible );

	if( m_width_attrib.m_seg_width.m_status < 0 )
	{
		m_width_attrib.m_seg_width.m_status = CInheritableInfo::E_USE_PARENT;

		m_radio1_default.SetCheck(1);
		m_combo_width.EnableWindow(0);
	}
	else
	{
		m_radio1_set.SetCheck(1);
		m_combo_width.EnableWindow(1);
	}
	str.Format("%d", m_width_attrib.m_seg_width.m_val / NM_PER_MIL);
	m_combo_width.SetWindowText( str );

	//BAF add E_USE_DEF_FROM_WIDTH to item types
	if( m_width_attrib.m_via_width.m_status < 0 )
	{
		m_width_attrib.m_via_width.m_status = CInheritableInfo::E_USE_PARENT;
		m_width_attrib.m_via_hole .m_status = CInheritableInfo::E_USE_PARENT;

		m_radio2_default.SetCheck(1);

		m_edit_pad_w.EnableWindow( 0 );
		m_edit_hole_w.EnableWindow( 0 );
	}
	else
	{
		m_radio2_set.SetCheck(1);

		m_edit_pad_w.EnableWindow( 1 );
		m_edit_hole_w.EnableWindow( 1 );
	}
	str.Format("%d", m_width_attrib.m_via_width.m_val / NM_PER_MIL );
	m_edit_pad_w.SetWindowText( str );
	str.Format("%d", m_width_attrib.m_via_hole.m_val / NM_PER_MIL );
	m_edit_hole_w.SetWindowText( str );


	if( m_width_attrib.m_ca_clearance.m_status < 0 )
	{
		m_width_attrib.m_ca_clearance.m_status = CInheritableInfo::E_USE_PARENT;

		m_radio3_default.SetCheck(1);
		m_edit_clearance.EnableWindow(0);
	}
	else
	{
		m_radio3_set.SetCheck(1);
		m_edit_clearance.EnableWindow(1);
	}
	str.Format("%d", m_width_attrib.m_ca_clearance.m_val / NM_PER_MIL);
	m_edit_clearance.SetWindowText(str);

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
		int npins = (*m_nl)[m_in].ref_des.GetSize();
		for( int i=0; i<npins; i++ )
		{
			str.Format( "%s.%s", (*m_nl)[m_in].ref_des[i], (*m_nl)[m_in].pin_name[i] );
			m_list_pins.InsertString( i, str );
		}
	}

	// default is to apply new trace width & clearances
	m_check_width_apply.SetCheck(1);
	m_check_via_apply.SetCheck(1);
	m_check_clearance_apply.SetCheck(1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgEditNet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Control(pDX, IDC_CHECK_VISIBLE, m_check_visible);

	DDX_Control(pDX, IDC_RADIO1_PROJ_DEF, m_radio1_default);
	DDX_Control(pDX, IDC_RADIO1_SET_TO, m_radio1_set);
	DDX_Control(pDX, IDC_COMBO_WIDTH, m_combo_width);
	DDX_Control(pDX, IDC_CHECK_TRACE, m_check_width_apply);

	DDX_Control(pDX, IDC_RADIO2_PROJ_DEF, m_radio2_default);
	DDX_Control(pDX, IDC_RADIO2_DEF_FOR_TRACE, m_radio2_def_for_width);
	DDX_Control(pDX, IDC_RADIO2_SET_TO, m_radio2_set);
	DDX_Control(pDX, IDC_EDIT_VIA_PAD_W, m_edit_pad_w);
	DDX_Control(pDX, IDC_EDIT_VIA_HOLE_W, m_edit_hole_w);
	DDX_Control(pDX, IDC_CHECK_VIA, m_check_via_apply);

	DDX_Control(pDX, IDC_RADIO3_PROJ_DEF, m_radio3_default);
	DDX_Control(pDX, IDC_RADIO3_SET_TO, m_radio3_set);
	DDX_Control(pDX, IDC_EDIT_CLEARANCE, m_edit_clearance);
	DDX_Control(pDX, IDC_CHECK_CLEARANCE, m_check_clearance_apply);

	DDX_Control(pDX, IDC_LIST_PIN, m_list_pins);
	DDX_Control(pDX, IDC_EDIT_ADD_PIN, m_edit_addpin);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add_pin);
	DDX_Control(pDX, ID_MY_OK, m_button_OK);

	if( pDX->m_bSaveAndValidate )
	{
		// now implement edits into netlist_info
		m_edit_name.GetWindowText( m_name );
		if( m_name.GetLength() > MAX_NET_NAME_SIZE )
		{
			CString mess;
			mess.Format( "Max length of net name is %d characters", MAX_NET_NAME_SIZE );
			AfxMessageBox( mess );
			pDX->Fail();
		}

		int i;

		if( m_radio1_default.GetCheck() )
		{
			m_width_attrib.m_seg_width.m_status = CInheritableInfo::E_USE_PARENT;
		}
		else
		{
			DDX_Text( pDX, IDC_COMBO_WIDTH, i );
			if( i<=0 || i > MAX_TRACE_W_MIL)
			{
				AfxMessageBox( "illegal trace width" );
				pDX->Fail();
			}
			m_width_attrib.m_seg_width = i*NM_PER_MIL;
		}

		if( m_radio2_default.GetCheck() )
		{
			m_width_attrib.m_via_width.m_status = CInheritableInfo::E_USE_PARENT;
			m_width_attrib.m_via_hole .m_status = CInheritableInfo::E_USE_PARENT;
		}
		else
		{
			DDX_Text( pDX, IDC_EDIT_VIA_PAD_W, i );
			if( i<=0 || i > MAX_VIA_W_MIL)
			{
				AfxMessageBox( "illegal via width" );
				pDX->Fail();
			}
			m_width_attrib.m_via_width = i*NM_PER_MIL;

			DDX_Text( pDX, IDC_EDIT_VIA_HOLE_W, i );
			if( i<=0 || i > MAX_VIA_HOLE_MIL)
			{
				AfxMessageBox( "illegal via hole width" );
				pDX->Fail();
			}
			m_width_attrib.m_via_hole = i*NM_PER_MIL;
		}

		m_visible = m_check_visible.GetState();
		for( int in=0; in<m_nl->GetSize(); in++ )
		{
			if( m_name == (*m_nl)[in].name && m_in != in && !(*m_nl)[in].deleted )
			{
				AfxMessageBox( "duplicate net name" );
				pDX->Fail();
			}
		}

		if( m_radio3_default.GetCheck() )
		{
			m_width_attrib.m_ca_clearance.m_status = CInheritableInfo::E_USE_PARENT;
		}
		else
		{
			DDX_Text( pDX, IDC_EDIT_CLEARANCE, i );
			if( i<0 ||  i>MAX_CLEARANCE_MIL )
			{
				CString str;
				str.Format( "Invalid clearance (0-%d)", MAX_CLEARANCE_MIL );
				AfxMessageBox( str );
				pDX->Fail();
			}

			m_width_attrib.m_ca_clearance = i*NM_PER_MIL;
		}

		// now update netlist_info
		if( m_new_net )
		{
			// add entry to netlist_info for new net, set index m_in
			int num_nets = m_nl->GetSize();
			m_nl->SetSize( num_nets + 1 );
			m_in = num_nets;
			(*m_nl)[m_in].net = NULL;
		}

		(*m_nl)[m_in].apply_trace_width = m_check_width_apply.GetCheck();
		(*m_nl)[m_in].apply_via_width   = m_check_via_apply.GetCheck();
		(*m_nl)[m_in].apply_clearance   = m_check_clearance_apply.GetCheck();
		(*m_nl)[m_in].modified = TRUE;
		(*m_nl)[m_in].name = m_name;
		(*m_nl)[m_in].visible = m_visible;
		(*m_nl)[m_in].width_attrib = m_width_attrib;

		if( m_pins_edited )
		{
			int npins = m_list_pins.GetCount();
			(*m_nl)[m_in].ref_des.SetSize( npins );
			(*m_nl)[m_in].pin_name.SetSize( npins );
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
				(*m_nl)[m_in].ref_des[i] = refstr;
				(*m_nl)[m_in].pin_name[i] = pinstr;
				// now remove pin from other net, if necessary
				for( int in=0; in<m_nl->GetSize(); in++ )
				{
					if( (*m_nl)[in].deleted == FALSE && m_in != in )
					{
						for( int ip=0; ip<(*m_nl)[in].ref_des.GetSize(); ip++ )
						{
							if( refstr == (*m_nl)[in].ref_des[ip]
							&& pinstr == (*m_nl)[in].pin_name[ip] )
							{
								(*m_nl)[in].modified = TRUE;
								(*m_nl)[in].ref_des.RemoveAt( ip );
								(*m_nl)[in].pin_name.RemoveAt( ip);
							}
						}
					}
				}
			}
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgEditNet, CDialog)
	ON_BN_CLICKED(IDC_RADIO1_PROJ_DEF, &CDlgEditNet::OnBnClickedRadio1ProjDef)
	ON_BN_CLICKED(IDC_RADIO1_SET_TO, &CDlgEditNet::OnBnClickedRadio1SetTo)
	ON_BN_CLICKED(IDC_RADIO2_PROJ_DEF, OnBnClickedRadio2DefWidth)
	ON_BN_CLICKED(IDC_RADIO2_SET_TO, OnBnClickedRadio2SetWidth)
	ON_BN_CLICKED(IDC_RADIO2_DEF_FOR_TRACE, OnBnClickedRadio2DefForTrace)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH, OnCbnSelchangeComboWidth)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchangeComboWidth)
	ON_EN_UPDATE(IDC_EDIT_ADD_PIN, OnEnUpdateEditAddPin)
	ON_BN_CLICKED(ID_MY_OK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO3_PROJ_DEF, &CDlgEditNet::OnBnClickedRadioDefClearance)
	ON_BN_CLICKED(IDC_RADIO3_SET_TO, &CDlgEditNet::OnBnClickedRadioSetClearance)
END_MESSAGE_MAP()


// CDlgEditNet message handlers

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
		AfxMessageBox( "Illegal pin" );
		return;
	}
	else
	{
		int test = m_list_pins.FindString( 0, str );
		if( test != -1 )
		{
			AfxMessageBox( "Pin already in this net" );
			return;
		}
		int dot_pos = str.FindOneOf( "." );
		if( dot_pos >= 2 && dot_pos < (len-1) )
		{
			CString refstr = str.Left( dot_pos );
			cpart * part = m_plist->GetPart( refstr );
			if( !part )
			{
				str.Format( "Part \"%s\" does not exist", refstr );
				AfxMessageBox( str );
				return;
			}
			if( !part->shape )
			{
				str.Format( "Part \"%s\" does not have a footprint", refstr );
				AfxMessageBox( str );
				return;
			}
			CString pinstr = str.Right( len - dot_pos - 1 );
			if( !CheckLegalPinName( &pinstr ) )
			{
				str = "Pin name must consist of zero or more letters\n";
				str	+= "Followed by zero or more numbers\n";
				str	+= "For example: 1, 23, A12, SOURCE are legal\n";
				str	+= "while 1A, A2B3 are not\n";
				AfxMessageBox( str );
				return;
			}
			int pin_index = part->shape->GetPinIndexByName( pinstr );
			if( pin_index == -1 )
			{
				str.Format( "Pin \"%s\" not found in footprint \"%s\"", pinstr,
					part->shape->m_name );
				AfxMessageBox( str );
				return;
			}
			// now search for pin in m_nl
			int i_found = -1;
			for( int i=0; i<m_nl->GetSize(); i++ )
			{
				if( (*m_nl)[i].deleted == FALSE )
				{
					for( int ip=0; ip<(*m_nl)[i].ref_des.GetSize(); ip++ )
					{
						if( refstr == (*m_nl)[i].ref_des[ip]
						&& pinstr == (*m_nl)[i].pin_name[ip] )
						{
							i_found = i;
							break;
						}
					}
				}
			}
			if( i_found != -1 )
			{
				if( i_found == m_in )
				{
					AfxMessageBox( "Pin already in this net" );
					return;
				}
				else
				{
					CString mess;
					mess.Format( "Pin %s.%s is assigned to net \"%s\"\nRemove it from \"%s\"? ",
						refstr, pinstr, (*m_nl)[i_found].name, (*m_nl)[i_found].name );
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

void CDlgEditNet::OnCbnSelchangeComboWidth()
{
	int i = m_combo_width.GetCurSel();

	CString text;
	m_combo_width.GetLBText( i, text );
	ChangeTraceWidth( atoi( (LPCSTR)text ) );
}

void CDlgEditNet::OnCbnEditchangeComboWidth()
{
	CString text;
	m_combo_width.GetWindowText( text );
	ChangeTraceWidth( atoi( (LPCSTR)text ) );
}


void CDlgEditNet::ChangeTraceWidth( int new_w )
{
	CString text;

	int n = m_w->GetSize();
	if( m_radio2_def_for_width.GetCheck() )
	{
		new_w *= NM_PER_MIL;
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
			text.Format( "%d", new_v_w/NM_PER_MIL );
			m_edit_pad_w.SetWindowText( text );
			text.Format( "%d", new_v_h_w/NM_PER_MIL );
			m_edit_hole_w.SetWindowText( text );
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


void CDlgEditNet::OnBnClickedRadio1ProjDef()
{
	m_combo_width.EnableWindow( 0 );

	m_width_attrib.m_seg_width.m_status = CInheritableInfo::E_USE_PARENT;
	m_width_attrib.Update_seg_width();

	CString str;
	int val = m_width_attrib.m_seg_width.m_val / NM_PER_MIL;
	str.Format("%d", val);
	m_combo_width.SetWindowText( str );

	ChangeTraceWidth( val );
}

void CDlgEditNet::OnBnClickedRadio1SetTo()
{
	m_combo_width.EnableWindow( 1 );
}


void CDlgEditNet::OnBnClickedRadio2DefWidth()
{
	m_edit_pad_w.EnableWindow( 0 );
	m_edit_hole_w.EnableWindow( 0 );

	m_width_attrib.m_via_width = CInheritableInfo::E_USE_PARENT;
	m_width_attrib.m_via_hole  = CInheritableInfo::E_USE_PARENT;
	m_width_attrib.Update_via_width();
	m_width_attrib.Update_via_hole();

	CString str;
	str.Format( "%d", m_width_attrib.m_via_width.m_val / NM_PER_MIL );
	m_edit_pad_w.SetWindowText( str );
	str.Format( "%d", m_width_attrib.m_via_hole.m_val / NM_PER_MIL );
	m_edit_hole_w.SetWindowText( str );
}

void CDlgEditNet::OnBnClickedRadio2DefForTrace()
{
	m_edit_pad_w.EnableWindow( 0 );
	m_edit_hole_w.EnableWindow( 0 );

	CString text;
	m_combo_width.GetWindowText( text );
	ChangeTraceWidth( atoi( (LPCSTR)text ) );
}

void CDlgEditNet::OnBnClickedRadio2SetWidth()
{
	m_edit_pad_w.EnableWindow( 1 );
	m_edit_hole_w.EnableWindow( 1 );
}


void CDlgEditNet::OnBnClickedRadioDefClearance()
{
	m_edit_clearance.EnableWindow(0);

	m_width_attrib.m_ca_clearance = CInheritableInfo::E_USE_PARENT;
	m_width_attrib.Update_ca_clearance();

	CString str;
	str.Format( "%d", m_width_attrib.m_ca_clearance.m_val / NM_PER_MIL );
	m_edit_clearance.SetWindowText( str );
}

void CDlgEditNet::OnBnClickedRadioSetClearance()
{
	m_edit_clearance.EnableWindow(1);
}
