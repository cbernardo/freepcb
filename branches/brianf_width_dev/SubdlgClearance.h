#pragma once

#ifndef _SUBDLGCLEARANCE_H /* [ */
#define _SUBDLGCLEARANCE_H

#include "pubsub.h"

class CSubDlg_Clearance
{
	int const m_AutoMode;

protected:
	void OnInitDialog(CInheritableInfo const &width_attrib);
	int  OnDDXOut();

	void OnChangeClearanceType();

protected:
	CButton m_rb_c_default;
	CButton m_rb_c_auto;
	CButton m_rb_c_set;
		CEdit m_edit_c_clearance;

	CButton m_check_c_modify;
	CButton m_check_c_apply;

protected:
	afx_msg void OnBnClicked_c_Default();
	afx_msg void OnBnClicked_c_Auto();
	afx_msg void OnBnClicked_c_Set();

	afx_msg void OnBnClicked_c_modify();

public:
	explicit CSubDlg_Clearance(int AutoMode);

	CClearanceInfo m_attrib;

	enum
	{
		E_NO_AUTO_MODE,
		E_USE_AUTO_MODE		
	};
};

#endif /* !_SUBDLGCLEARANCE_H ] */
