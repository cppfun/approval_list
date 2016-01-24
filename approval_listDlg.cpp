// approval_listDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "approval_list.h"
#include "approval_listDlg.h"
#include "WebGrab.h"
#include "AClass/LiteHTMLReader.h"
#include "utfconv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Capproval_listDlg �Ի���




Capproval_listDlg::Capproval_listDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Capproval_listDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Capproval_listDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Capproval_listDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &Capproval_listDlg::OnBnClickedOk)
	ON_EN_SETFOCUS(IDC_EDIT_OUTPUT, &Capproval_listDlg::OnEnSetfocusEditOutput)
END_MESSAGE_MAP()


// Capproval_listDlg ��Ϣ�������

BOOL Capproval_listDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetDlgItemText(IDC_EDIT_url,_T("http://www.gzegn.gov.cn/xzspindex.jspx?areaId=520200"));


	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Capproval_listDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR Capproval_listDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void Capproval_listDlg::OnBnClickedOk()
{
	CString szUrl;
	GetDlgItemText(IDC_EDIT_url,szUrl);
	if(szUrl.IsEmpty())
		return;

	CWebGrab grab;
	//set all params
	grab.SetTimeOut(2000);
	//call init
	grab.Initialise(_T("Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/44.0.2403.155 Safari/537.36"),NULL);

	CString szBuff;

	if(!grab.GetFile(szUrl, szBuff, _T("Opera"),NULL )) {
		SetDlgItemText(IDC_STATIC_TIP,_T("��ȡ��������ʧ�ܣ����Ժ�����..."));
		return;
	}
	SetDlgItemText(IDC_STATIC_TIP,_T("��ȡ�������ݳɹ������ڽ���ת�봦��..."));
#ifdef _UNICODE
	CString szPageW = UTF8Util::ConvertUTF8ToUTF16((char*)szBuff.GetBuffer());
	m_szHtmlPage = szPageW.GetBuffer();
#else
	m_szHtmlPage = szBuff;
#endif
	SetDlgItemText(IDC_STATIC_TIP,_T("��ʼ��ʽ����������..."));
	m_szHtmlPage=CleanHtml(m_szHtmlPage);
	SetDlgItemText(IDC_STATIC_TIP,_T("��ʽ���������ݽ���..."));
	SetOutputHtml(_T("div"), _T("id"), _T("demo1"));
	CString szTxt((LPCTSTR)m_szHtmlPage+m_dwarrTagStart.GetAt(0),m_dwarrTagLen.GetAt(0));
	// result = result.replace('href="', 'target="_blank" href="http://www.gzegn.gov.cn')
	SetDlgItemText(IDC_STATIC_TIP,_T("�ɹ���ȡĿ�����ݣ�������..."));
	szTxt.Replace(_T("a "),_T("a target='_blank' "));
	szTxt.Replace(_T("/queryBusinessinfo"), _T("http://www.gzegn.gov.cn/queryBusinessinfo"));
	SetDlgItemText(IDC_EDIT_OUTPUT, szTxt);

}

CString Capproval_listDlg::CleanHtml(const char* input)
{
	CString result;
	//CStringA input(html);
	TidyBuffer output = {0};
	TidyBuffer errbuf = {0};
	int rc = -1;
	Bool ok;

	TidyDoc tdoc = tidyCreate();                     // Initialize "document"

	ok = tidyOptSetBool( tdoc, TidyXhtmlOut, no );  // Convert to XHTML
	if ( ok )
		rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
	/*if (rc>=0) {
		rc = tidyOptSetInt(tdoc, TidyOutCharEncoding, 0);
	}*/
	if ( rc >= 0 )
		rc = tidyParseString( tdoc, input );           // Parse the input
	if ( rc >= 0 )
		rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
	if ( rc >= 0 )
		rc = tidyRunDiagnostics( tdoc );               // Kvetch
	if ( rc > 1 )                                    // If error, force output.
		rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
	if ( rc >= 0 )
		rc = tidySaveBuffer( tdoc, &output );          // Pretty Print

	if ( rc >= 0 )
		result=output.bp;
	else {
		CString orign(input);
		result=orign;
	}

	tidyBufFree( &output );
	tidyBufFree( &errbuf );
	tidyRelease( tdoc );
	return result;
}

CString Capproval_listDlg::CleanHtml(CString input)
{
	CString result;
	CStringA ainput(input);
	TidyBuffer output = {0};
	TidyBuffer errbuf = {0};
	int rc = -1;
	Bool ok;

	TidyDoc tdoc = tidyCreate();                     // Initialize "document"

	ok = tidyOptSetBool( tdoc, TidyXhtmlOut, no );  // Convert to XHTML
	if ( ok )
		rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
	if (rc>=0) {
		rc = tidyOptSetInt(tdoc, TidyOutCharEncoding, 0);
	}
	if ( rc >= 0 )
		rc = tidyParseString( tdoc, ainput );           // Parse the input
	if ( rc >= 0 )
		rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
	if ( rc >= 0 )
		rc = tidyRunDiagnostics( tdoc );               // Kvetch
	if ( rc > 1 )                                    // If error, force output.
		rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
	if ( rc >= 0 )
		rc = tidySaveBuffer( tdoc, &output );          // Pretty Print

	if ( rc >= 0 )
		result=output.bp;
	else 
		result=input;

	tidyBufFree( &output );
	tidyBufFree( &errbuf );
	tidyRelease( tdoc );
	return result;
}

void Capproval_listDlg::SetOutputHtml(CString szTag, CString szAtrib, CString szAtribVal)
{
	if(m_szHtmlPage.IsEmpty()){
		SetDlgItemText(IDC_STATIC_TIP,_T("��ȡ��������ʧ�ܣ����Ժ�����"));
		return;

	}
	CLiteHTMLReader theReader;
	CHtmlElementCollection theElementCollectionHandler;
	theReader.setEventHandler(&theElementCollectionHandler);
	SetDlgItemText(IDC_STATIC_TIP,_T("׼����ȡĿ������..."));
	theElementCollectionHandler.InitWantedTag(szTag,szAtrib,szAtribVal);//style

	if(theReader.Read(m_szHtmlPage)){
		int iNoElements = theElementCollectionHandler.GetNumElements();
		int ift = theElementCollectionHandler.GetNumElementsFiltered();

		HtmlTree hTree =  theElementCollectionHandler.GetTree();
		FillList(hTree);
	}
}
static int giElements = 0;
void Capproval_listDlg::FillList(HtmlTree hTree)
{
	giElements = 0;
	m_dwarrTagStart.RemoveAll();
	m_dwarrTagLen.RemoveAll();

	AddTagToList(hTree);
}

BOOL Capproval_listDlg::AddTagToList(HtmlNode node)
{
	int iElements = node.Count;
	for(int n=0;n<iElements;n++) {
		if(!node.Nodes[n]->iFiltered){
			DWORD dwStart = node.Nodes[n]->lpszStartStart - m_szHtmlPage;
			m_dwarrTagStart.Add(dwStart);
			//elements with end tag
			if(node.Nodes[n]->iComplete){
				DWORD dwLen = node.Nodes[n]->lpszStopStop - node.Nodes[n]->lpszStartStart;
				m_dwarrTagLen.Add(dwLen);
			}
			////elements with no ending tag
			else{
				DWORD dwLen = node.Nodes[n]->lpszStartStop - node.Nodes[n]->lpszStartStart;
				m_dwarrTagLen.Add(dwLen);
			}

			//MAKELONG(dwStart,dwLen)
			giElements++;	
		}

		if(!node.Nodes[n].IsLeaf())
			AddTagToList(node.Nodes[n]);
	}

	return FALSE;
}

void Capproval_listDlg::OnEnSetfocusEditOutput()
{
	CEdit* p_output = (CEdit*)GetDlgItem(IDC_EDIT_OUTPUT);
	p_output->SetSel(0, -1);
	p_output=NULL;
}
