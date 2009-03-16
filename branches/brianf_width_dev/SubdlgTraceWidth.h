#pragma once

#ifndef _SUBDLGTRACEWIDTH_H /* [ */
#define _SUBDLGTRACEWIDTH_H

#include "pubsub.h"

class CSubDlg_TraceWidth
{
	void ChangeTraceWidth( int new_w );

protected:
	void OnInitDialog(CInheritableInfo const &width_attrib);
	int  OnDDXOut();

	void OnChangeWidthType();

protected:
	CButton   m_rb_t_default;
	CButton   m_rb_t_set;
		CComboBox m_combo_t_width;

	CButton   m_check_t_modify;
	CButton   m_check_t_apply;

protected:
	afx_msg void OnBnClicked_t_Default();
	afx_msg void OnBnClicked_t_Set();

	afx_msg void OnCbnSelchange_t_width();
	afx_msg void OnCbnEditchange_t_width();

	afx_msg void OnBnClicked_t_modify();

public:
	CSubDlg_TraceWidth();

	CPublishedData<CConnectionWidthInfo> m_attrib;

	CArray<int> *m_w;      // array of default widths
	CArray<int> *m_v_w;    // array of via widths (matching m_w[] entries)
	CArray<int> *m_v_h_w;  // array of via hole widths
};

#endif /* !_SUBDLGTRACEWIDTH_H ] */
