// CDlgNetCombine dialog
// CPT2 rewrote pretty completely because this is now a subsidiary dialog for the main CDlgNetlist.

class CDlgNetCombine : public CDialog
{
	DECLARE_DYNAMIC(CDlgNetCombine)

public:
	CDlgNetCombine(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgNetCombine();

// Dialog Data
	enum { IDD = IDD_NET_COMBINE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CArray<CString> *m_names;	// array of net names to combine
	CArray<int> *m_pins;		// array of pin counts
	CString m_new_name;			// name for combined net (on exit)

	void Initialize(CArray<CString> *names, CArray<int> *pins);
private:
	CListCtrl m_list_ctrl;
	virtual BOOL OnInitDialog();
	void DrawListCtrl();
};
