#pragma once

#ifndef _SUBDLGVIAWIDTH_H /* [ */
#define _SUBDLGVIAWIDTH_H

#include "pubsub.h"

class CSubDlg_TraceWidth;

class CSubDlg_ViaWidth
{
	CSubDlg_TraceWidth *my_SubDlg_TraceWidth;

	class CTraceWidthUpdate : public CDataSubscriber<CConnectionWidthInfo>
	{
		void Update( CConnectionWidthInfo const &width_attrib );
	} m_TraceWidthUpdate;
	friend class CTraceWidthUpdate;

	void TraceWidthUpdate( CConnectionWidthInfo const &width_attrib );

	void OnChangeWidthType();

protected:
	void OnInitDialog( CInheritableInfo const &width_attrib );
	int  OnDDXOut();

protected:
	CButton m_rb_v_default;
	CButton m_rb_v_def_for_width;
	CButton m_rb_v_set;
		CStatic m_text_v_pad_w;
		CStatic m_text_v_hole_w;
		CEdit m_edit_v_pad_w;
		CEdit m_edit_v_hole_w;

	CButton m_check_v_modify;
	CButton m_check_v_apply;

protected:
	afx_msg void OnBnClicked_v_Default();
	afx_msg void OnBnClicked_v_DefForTrace();
	afx_msg void OnBnClicked_v_Set();

	afx_msg void OnBnClicked_v_modify();

protected:
	explicit CSubDlg_ViaWidth( CSubDlg_TraceWidth *pSubDlg_TraceWidth = NULL );

	CConnectionWidthInfo m_attrib;

public:
	CConnectionWidthInfo const &get_attrib() const { return m_attrib; }
};

#endif /* !_SUBDLGVIAWIDTH_H ] */
