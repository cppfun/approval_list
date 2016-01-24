// approval_listDlg.h : ͷ�ļ�
//
#include "AClass/HtmlElementCollection.h"
#pragma once


// Capproval_listDlg �Ի���
class Capproval_listDlg : public CDialog
{
// ����
public:
	Capproval_listDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_APPROVAL_LIST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CString CleanHtml(const char* input);
	CString CleanHtml(CString input);
	CDWordArray m_dwarrTagStart;
	CDWordArray m_dwarrTagLen;
private:
	CString m_szHtmlPage;
private:
	void SetOutputHtml(CString szTag, CString szAtrib, CString szAtribVal);
	void FillList(HtmlTree hTree);
	BOOL AddTagToList(HtmlNode node);
	afx_msg void OnEnSetfocusEditOutput();
};
