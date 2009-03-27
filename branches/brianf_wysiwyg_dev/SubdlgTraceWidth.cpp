#include "stdafx.h"
#include "SubdlgTraceWidth.h"

CSubDlg_TraceWidth::CSubDlg_TraceWidth()
{
	m_w = NULL;
}

void CSubDlg_TraceWidth::OnInitDialog(CInheritableInfo const &width_attrib)
{
	CString str;

	m_attrib.get_data().Undef();
	m_attrib.get_data() = width_attrib;
	m_attrib.get_data().Update();

	if( m_w )
	{
		for( int iw=0; iw < m_w->GetSize(); iw++ )
		{
			int w = (*m_w)[iw];
			str.Format( "%d", w/NM_PER_MIL );
			m_combo_t_width.InsertString( iw, str );
		}
	}

	if( !m_attrib.get_data().m_seg_width.isDefined() )
	{
		m_text_t_group.EnableWindow( 0 );
		m_check_t_modify.SetCheck( 0 );
		m_check_t_modify.EnableWindow( 0 );
	}
	else 
	{
		if( m_attrib.get_data().m_seg_width.m_status < 0 )
		{
			m_attrib.get_data().m_seg_width.m_status = CInheritableInfo::E_USE_PARENT;

			m_rb_t_default.SetCheck(1);
		}
		else
		{
			m_rb_t_set.SetCheck(1);
		}

		str.Format("%d", m_attrib.get_data().m_seg_width.m_val / NM_PER_MIL);
		m_combo_t_width.SetWindowText( str );
	}

	OnChangeWidthType();
	OnBnClicked_t_modify();
}

int CSubDlg_TraceWidth::OnDDXOut()
{
	if( m_check_t_modify.GetCheck() )
	{
		if( m_rb_t_default.GetCheck() )
		{
			m_attrib.get_data().m_seg_width.m_status = CInheritableInfo::E_USE_PARENT;
		}
		else
		{
			CString str;

			m_combo_t_width.GetWindowText( str );

			int trace_w;
			if( (sscanf(str, "%d", &trace_w) != 1) || (trace_w < 1) || (trace_w > MAX_TRACE_W_MIL) )
			{
				str.Format( "illegal trace width (1 - %d)", MAX_TRACE_W_MIL );
				AfxMessageBox( str );

				return 0;
			}
			m_attrib.get_data().m_seg_width = trace_w * NM_PER_MIL;
		}
	}
	else
	{
		m_attrib.get_data().m_seg_width.Undef();

		m_check_t_apply.SetCheck( 0 );
	}

	// Undef any non-trace items
	m_attrib.get_data().m_via_width.Undef();
	m_attrib.get_data().m_via_hole.Undef();

	return 1;
}


void CSubDlg_TraceWidth::OnBnClicked_t_Default()
{
	OnChangeWidthType();

	m_attrib.get_data().m_seg_width.m_status = CInheritableInfo::E_USE_PARENT;
	m_attrib.get_data().Update_seg_width();

	CString str;
	int val = m_attrib.get_data().m_seg_width.m_val / NM_PER_MIL;
	str.Format("%d", val);
	m_combo_t_width.SetWindowText( str );

	ChangeTraceWidth( val );
}

void CSubDlg_TraceWidth::OnBnClicked_t_Set()
{
	OnChangeWidthType();
}

void CSubDlg_TraceWidth::OnCbnSelchange_t_width()
{
	int i = m_combo_t_width.GetCurSel();

	CString text;
	m_combo_t_width.GetLBText( i, text );
	ChangeTraceWidth( atoi( (LPCSTR)text ) );
}

void CSubDlg_TraceWidth::OnCbnEditchange_t_width()
{
	CString text;
	m_combo_t_width.GetWindowText( text );
	ChangeTraceWidth( atoi( (LPCSTR)text ) );
}


void CSubDlg_TraceWidth::ChangeTraceWidth( int new_w )
{
	// Convert units
	new_w *= NM_PER_MIL;

	m_attrib.get_data().m_seg_width = new_w;
	m_attrib.get_data().Update_seg_width();

	// Publish the changes
	m_attrib.Update();
}

void CSubDlg_TraceWidth::OnBnClicked_t_modify()
{
	if( !m_check_t_modify.GetCheck() )
	{
		m_rb_t_default.EnableWindow( 0 );
		m_rb_t_set.EnableWindow( 0 );

		m_check_t_apply.EnableWindow( 0 );

		m_combo_t_width.EnableWindow( 0 );
	}
	else
	{
		m_rb_t_default.EnableWindow( m_attrib.get_data().hasParent() );
		m_rb_t_set.EnableWindow( 1 );

		m_check_t_apply.EnableWindow( 1 );

		OnChangeWidthType();
	}
}

void CSubDlg_TraceWidth::OnChangeWidthType()
{
	m_combo_t_width.EnableWindow( m_rb_t_set.GetCheck() ? 1 : 0 );
}
