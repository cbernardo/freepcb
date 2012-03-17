// DlgPrefs.cpp : implementation file.  ALL CPT.

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPrefs.h"
#include "afxdialogex.h"


// CDlgPrefs dialog

IMPLEMENT_DYNAMIC(CDlgPrefs, CDialogEx)

CDlgPrefs::CDlgPrefs(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgPrefs::IDD, pParent)
{

}

CDlgPrefs::~CDlgPrefs()
{
}

void CDlgPrefs::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REVERSE_PGUP_PGDN, m_check_reverse);
	DDX_Control(pDX, IDC_CHECK_GRD1, m_check_grd1);
	DDX_Control(pDX, IDC_CHECK_GRD2, m_check_grd2);
	DDX_Control(pDX, IDC_CHECK_GRD3, m_check_grd3);
	DDX_Control(pDX, IDC_CHECK_GRD4, m_check_grd4);
	DDX_Control(pDX, IDC_CHECK_GRD5, m_check_grd5);
	DDX_Control(pDX, IDC_CHECK_GRD6, m_check_grd6);
	DDX_Control(pDX, IDC_CHECK_GRD7, m_check_grd7);
	DDX_Control(pDX, IDC_CHECK_GRD8, m_check_grd8);
	DDX_Control(pDX, IDC_CHECK_GRD9, m_check_grd9);
	DDX_Control(pDX, IDC_CHECK_GRD10, m_check_grd10);
	DDX_Control(pDX, IDC_CHECK_GRD11, m_check_grd11);
	DDX_Control(pDX, IDC_CHECK_GRD12, m_check_grd12);
	DDX_Control(pDX, IDC_CHECK_GRD13, m_check_grd13);
	DDX_Control(pDX, IDC_CHECK_GRD14, m_check_grd14);
	DDX_Control(pDX, IDC_CHECK_GRD15, m_check_grd15);
	DDX_Control(pDX, IDC_CHECK_GRD16, m_check_grd16);
	DDX_Control(pDX, IDC_CHECK_GRD17, m_check_grd17);
	DDX_Control(pDX, IDC_CHECK_GRD18, m_check_grd18);
	DDX_Control(pDX, IDC_CHECK_GRD19, m_check_grd19);
	DDX_Control(pDX, IDC_CHECK_GRD20, m_check_grd20);
	DDX_Control(pDX, IDC_CHECK_GRD21, m_check_grd21);
	DDX_Control(pDX, IDC_CHECK_GRD22, m_check_grd22);
	DDX_Control(pDX, IDC_CHECK_GRD23, m_check_grd23);
	DDX_Control(pDX, IDC_CHECK_GRD24, m_check_grd24);
	DDX_Control(pDX, IDC_CHECK_GRD25, m_check_grd25);
	DDX_Control(pDX, IDC_CHECK_GRD26, m_check_grd26);
	DDX_Control(pDX, IDC_CHECK_GRD27, m_check_grd27);
	DDX_Control(pDX, IDC_CHECK_GRD28, m_check_grd28);
	DDX_Control(pDX, IDC_CHECK_GRD29, m_check_grd29);
	DDX_Control(pDX, IDC_CHECK_GRD30, m_check_grd30);

	if( !pDX->m_bSaveAndValidate )	{
		// incoming
		m_check_reverse.SetCheck(fReverse);
		m_check_grd1.SetCheck(fGridFlags[0]);
		m_check_grd2.SetCheck(fGridFlags[1]);
		m_check_grd3.SetCheck(fGridFlags[2]);
		m_check_grd4.SetCheck(fGridFlags[3]);
		m_check_grd5.SetCheck(fGridFlags[4]);
		m_check_grd6.SetCheck(fGridFlags[5]);
		m_check_grd7.SetCheck(fGridFlags[6]);
		m_check_grd8.SetCheck(fGridFlags[7]);
		m_check_grd9.SetCheck(fGridFlags[8]);
		m_check_grd10.SetCheck(fGridFlags[9]);
		m_check_grd11.SetCheck(fGridFlags[10]);
		m_check_grd12.SetCheck(fGridFlags[11]);
		m_check_grd13.SetCheck(fGridFlags[12]);
		m_check_grd14.SetCheck(fGridFlags[13]);
		m_check_grd15.SetCheck(fGridFlags[14]);
		m_check_grd16.SetCheck(fGridFlags[15]);
		m_check_grd17.SetCheck(fGridFlags[16]);
		m_check_grd18.SetCheck(fGridFlags[17]);
		m_check_grd19.SetCheck(fGridFlags[18]);
		m_check_grd20.SetCheck(fGridFlags[19]);
		m_check_grd21.SetCheck(fGridFlags[20]);
		m_check_grd22.SetCheck(fGridFlags[21]);
		m_check_grd23.SetCheck(fGridFlags[22]);
		m_check_grd24.SetCheck(fGridFlags[23]);
		m_check_grd25.SetCheck(fGridFlags[24]);
		m_check_grd26.SetCheck(fGridFlags[25]);
		m_check_grd27.SetCheck(fGridFlags[26]);
		m_check_grd28.SetCheck(fGridFlags[27]);
		m_check_grd29.SetCheck(fGridFlags[28]);
		m_check_grd30.SetCheck(fGridFlags[29]);
		}
	else { // outgoing
		fReverse = m_check_reverse.GetCheck();
		fGridFlags[0] = m_check_grd1.GetCheck();
		fGridFlags[1] = m_check_grd2.GetCheck();
		fGridFlags[2] = m_check_grd3.GetCheck();
		fGridFlags[3] = m_check_grd4.GetCheck();
		fGridFlags[4] = m_check_grd5.GetCheck();
		fGridFlags[5] = m_check_grd6.GetCheck();
		fGridFlags[6] = m_check_grd7.GetCheck();
		fGridFlags[7] = m_check_grd8.GetCheck();
		fGridFlags[8] = m_check_grd9.GetCheck();
		fGridFlags[9] = m_check_grd10.GetCheck();
		fGridFlags[10] = m_check_grd11.GetCheck();
		fGridFlags[11] = m_check_grd12.GetCheck();
		fGridFlags[12] = m_check_grd13.GetCheck();
		fGridFlags[13] = m_check_grd14.GetCheck();
		fGridFlags[14] = m_check_grd15.GetCheck();
		fGridFlags[15] = m_check_grd16.GetCheck();
		fGridFlags[16] = m_check_grd17.GetCheck();
		fGridFlags[17] = m_check_grd18.GetCheck();
		fGridFlags[18] = m_check_grd19.GetCheck();
		fGridFlags[19] = m_check_grd20.GetCheck();
		fGridFlags[20] = m_check_grd21.GetCheck();
		fGridFlags[21] = m_check_grd22.GetCheck();
		fGridFlags[22] = m_check_grd23.GetCheck();
		fGridFlags[23] = m_check_grd24.GetCheck();
		fGridFlags[24] = m_check_grd25.GetCheck();
		fGridFlags[25] = m_check_grd26.GetCheck();
		fGridFlags[26] = m_check_grd27.GetCheck();
		fGridFlags[27] = m_check_grd28.GetCheck();
		fGridFlags[28] = m_check_grd29.GetCheck();
		fGridFlags[29] = m_check_grd30.GetCheck();
		}
	}


BEGIN_MESSAGE_MAP(CDlgPrefs, CDialogEx)
END_MESSAGE_MAP()


// CDlgPrefs message handlers
