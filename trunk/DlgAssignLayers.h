#pragma once


// CDlgAssignLayers dialog

class CDlgAssignLayers : public CDialog
{
	DECLARE_DYNAMIC(CDlgAssignLayers)

public:
	CDlgAssignLayers(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAssignLayers();

// Dialog Data
	enum { IDD = IDD_MOVE_LAYERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
