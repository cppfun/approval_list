// MyInternetSession.cpp : implementation file
//

#include "stdafx.h"
#include "WebGrab.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BUFFER_SIZE 4095

/////////////////////////////////////////////////////////////////////////////
// CWebGrab

CWebGrab::CWebGrab()
{
    m_pSession = NULL;
	m_timeOut = 0;
	m_bForceReload = true;
	m_useProxy = false;
	m_dwFileSz = 0;
	m_pProgressCtrl = 0;
}

CWebGrab::~CWebGrab()
{
	Close();
}

BOOL CWebGrab::Initialise(LPCTSTR szAgentName /*=NULL*/, CWnd* pWnd /*=NULL*/)
{
    Close();

    m_pSession = new CWebGrabSession(szAgentName, pWnd);

	if (m_timeOut != 0)
		m_pSession->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT  ,m_timeOut);

	if (m_useProxy){
		char buf[10];
		_itoa_s(m_Port,buf,10,10);
		CString temp = m_Proxy+(CString)":"+(CString)buf;
		INTERNET_PROXY_INFO proxyinfo;
		proxyinfo.dwAccessType = INTERNET_OPEN_TYPE_PROXY;
		proxyinfo.lpszProxy = temp;
		proxyinfo.lpszProxyBypass = NULL;
		m_pSession->SetOption(INTERNET_OPTION_PROXY, (LPVOID)&proxyinfo, sizeof(INTERNET_PROXY_INFO));
	}

    return (m_pSession != NULL);
}

void CWebGrab::Close()
{
    if (m_pSession){
        delete m_pSession;
		m_pSession = NULL;
    }
}

void CWebGrab::SetTimeOut(DWORD timeOut)
{
	m_timeOut = timeOut;
}
double CWebGrab::GetRate()
{
	return m_transferRate;
}

void CWebGrab::SetProxyServer(LPCSTR server)
{
	m_Proxy = server;
}

void CWebGrab::SetProxyPort(UINT port)
{
	m_Port = port;
}

void CWebGrab::SetUseProxy(bool use)
{
	m_useProxy = use;
}

void CWebGrab::SetProxy(LPCSTR proxy, WORD port, bool useProxy )
{
	SetProxyServer(proxy);
	SetProxyPort(port);
	SetUseProxy(useProxy);
}

void CWebGrab::SetProgressCtrl(CProgressCtrl *pProgressCtrl)
{
	m_pProgressCtrl = pProgressCtrl;
}

BOOL CWebGrab::GetFile(LPCTSTR szURL, CString& szBuffer,  LPCTSTR szAgentName /*=NULL*/, CWnd* pWnd /*=NULL*/)
{
    //TRACE1("URL is %s\n", szURL);
	CStringA strBuffer;

    if (!m_pSession && !Initialise(szAgentName, pWnd))
        return FALSE;

    if (pWnd)
        m_pSession->SetStatusWnd(pWnd);

    m_pSession->SetStatus(_T("Downloading file..."));
	
    DWORD dwCount = 0;
    CHttpFile* pFile = NULL;
    try
    {
        DWORD dwFlags = INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_DONT_CACHE;
        if (m_bForceReload)
            dwFlags |= INTERNET_FLAG_RELOAD;

        pFile = (CHttpFile*) m_pSession->OpenURL(szURL,1,dwFlags);
    }
    catch (CInternetException* e)
    {
        TCHAR   szCause[255];
        e->GetErrorMessage(szCause, 255);
        m_pSession->SetStatus(szCause);
        // e->ReportError();
        e->Delete();
		if(pFile)
			delete pFile;
        pFile = NULL;
        return FALSE;
    }
    
	// Get document size - return on error
	m_dwFileSz = 0;
	if(!pFile->QueryInfo (HTTP_QUERY_CONTENT_LENGTH, m_dwFileSz))
		m_dwFileSz = 0;
	BOOL bShPrg = 0;
	if(m_pProgressCtrl&&m_dwFileSz){
		bShPrg = 1;
		m_pProgressCtrl->SetRange32(0,m_dwFileSz);
		m_pProgressCtrl->SetPos(0);
		m_pProgressCtrl->SetStep(1);
	}

    COleDateTime startTime = COleDateTime::GetCurrentTime();
//    LPSTR buffer = NULL;
    if (pFile) {
        LPBYTE buffer[BUFFER_SIZE];
        if (!buffer) {
            pFile->Close();
            delete pFile;
            return FALSE;
        }

        try {
            UINT nRead = 0;
            dwCount = 0;
            do
            {
                nRead = pFile->Read(buffer, BUFFER_SIZE);
                if (nRead > 0)
                {
					if(bShPrg)
						m_pProgressCtrl->SetPos(dwCount+nRead);
					//if(nRead<BUFFER_SIZE)
					//	buffer[nRead] = 0;

                    LPBYTE ptr = (LPBYTE)strBuffer.GetBufferSetLength(dwCount + nRead);
                    memcpy(ptr+dwCount, buffer, nRead);

                    dwCount += nRead;
                    strBuffer.ReleaseBuffer(dwCount);

                    COleDateTimeSpan elapsed = COleDateTime::GetCurrentTime() - startTime;
                    double dSecs = elapsed.GetTotalSeconds();
                    if (dSecs > 0.0){
						m_transferRate = (double)dwCount / 1024.0 / dSecs;
                        m_pSession->SetStatus(_T("Read %d bytes (%0.1f Kb/s)"),dwCount, m_transferRate);
					}
                    else{
                        m_pSession->SetStatus(_T("Read %d bytes"), dwCount);
						m_transferRate = dwCount;
					}

                }
            }
            while (nRead > 0);
        }
        catch (CInternetException *e)
        {
            TCHAR   szCause[255];
            e->GetErrorMessage(szCause, 255);
            m_pSession->SetStatus(szCause);
            //e->ReportError();
            e->Delete();
            delete pFile;
            //::GlobalFree(buffer);
            return FALSE;
        }
               
        pFile->Close();
		//::GlobalFree(buffer);
        delete pFile;
    }

    m_pSession->SetStatus(_T(""));

	szBuffer.Empty();
	LPBYTE ptr = (LPBYTE)szBuffer.GetBufferSetLength(dwCount+sizeof(TCHAR));
	memcpy(ptr, strBuffer.GetBuffer(), dwCount);
	((TCHAR*)&(ptr[dwCount]))[0] = (TCHAR)0x0;
	szBuffer.ReleaseBuffer(dwCount);
	//g_Global.LOG(szBuffer,"pday.html");

    return TRUE;
}
BOOL CWebGrab::GetFileInfo(LPCTSTR  szURL,CString& strLastModified,DWORD& dwSize,
				DWORD& dwServerError,LPCTSTR szAgentName,CWnd* pWnd)
{
	// Initialize
	dwServerError = 0;

	if (!m_pSession && !Initialise(szAgentName, pWnd)) {
		return FALSE;
	}
	
	if (pWnd != NULL) {
		m_pSession->SetStatusWnd (pWnd);
	}
	
	// Open URL
	CHttpFile* pFile = NULL;
	try {
		DWORD dwFlags = INTERNET_FLAG_TRANSFER_BINARY 
			//| INTERNET_OPEN_FLAG_USE_EXISTING_CONNECT |
			//| INTERNET_FLAG_DONT_CACHE
			//| INTERNET_FLAG_RELOAD
			;
		if (m_bForceReload) {
					dwFlags |= INTERNET_FLAG_RELOAD;
		 		}
				
		pFile = (CHttpFile*) m_pSession->OpenURL(szURL, 1, dwFlags);
	}
	catch (CInternetException* e) {
		TCHAR   szCause[255];
		e->GetErrorMessage(szCause, 255);
		m_pSession->SetStatus(szCause);
		e->Delete();
		delete pFile;
		pFile = NULL;
		return FALSE;
	}
	
	// Get modification timestamp - return on error
	strLastModified.Empty();
	BOOL bStatus = pFile->QueryInfo (HTTP_QUERY_LAST_MODIFIED, strLastModified);
	if (!bStatus) {
		pFile->QueryInfoStatusCode (dwServerError);
		if (dwServerError > 399) {
			pFile->Close();
			delete pFile;
			return (FALSE);
		}
	}
	
	// Get document size - return on error
	dwSize = 0;
	bStatus = pFile->QueryInfo (HTTP_QUERY_CONTENT_LENGTH, dwSize);
	if (!bStatus) {
		pFile->QueryInfoStatusCode (dwServerError);
		if (dwServerError > 399) {
			pFile->Close();
			delete pFile;
			return (FALSE);
		}
	}
	
	// Close URL and return successfully
	pFile->Close();
	delete pFile;
	return (TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CWebGrabSession

CWebGrabSession::CWebGrabSession(LPCTSTR szAgentName) 
                : CInternetSession(szAgentName) // , 1, INTERNET_OPEN_TYPE_PRECONFIG, 
{
    CommonConstruct();
}

CWebGrabSession::CWebGrabSession(LPCTSTR szAgentName, CWnd* pStatusWnd) 
                : CInternetSession(szAgentName) //, 1, INTERNET_OPEN_TYPE_PRECONFIG, 
                //                  NULL, NULL, INTERNET_FLAG_ASYNC)
{
    CommonConstruct();
    m_pStatusWnd = pStatusWnd;
}

CWebGrabSession::~CWebGrabSession()
{
}

void CWebGrabSession::CommonConstruct() 
{
    m_pStatusWnd = NULL;
    try {
        EnableStatusCallback(TRUE);
    }
    catch (...)
    {}
}
// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CWebGrabSession, CInternetSession)
	//{{AFX_MSG_MAP(CWebGrabSession)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// CWebGrabSession member functions

void CWebGrabSession::OnStatusCallback(DWORD dwContext, 
                                       DWORD dwInternetStatus, 
                                       LPVOID lpvStatusInformation, 
                                       DWORD dwStatusInformationLength)
{
    // Status callbacks need thread-state protection. 
    AFX_MANAGE_STATE( AfxGetAppModuleState( ) );

    CString str;

	//TRACE1("Internet context=%d: %d\n", dwContext);

	switch (dwInternetStatus)
	{
	case INTERNET_STATUS_RESOLVING_NAME:
		str.Format(_T("Resolving name for %s"), lpvStatusInformation);
		break;

	case INTERNET_STATUS_NAME_RESOLVED:
		str.Format(_T("Resolved name for %s"), lpvStatusInformation);
		break;

	case INTERNET_STATUS_HANDLE_CREATED:
		//str.Format("Handle %8.8X created", hInternet);
		break;

	case INTERNET_STATUS_CONNECTING_TO_SERVER:
		{
		//sockaddr* pSockAddr = (sockaddr*) lpvStatusInformation;
		str.Format(_T("Connecting to socket address ")); //, pSockAddr->sa_data);
		}
		break;

	case INTERNET_STATUS_REQUEST_SENT:
		str.Format(_T("Request sent"));
		break;

	case INTERNET_STATUS_SENDING_REQUEST:
		str.Format(_T("Sending request..."));
		break;

	case INTERNET_STATUS_CONNECTED_TO_SERVER:
		str.Format(_T("Connected to socket address"));
		break;

	case INTERNET_STATUS_RECEIVING_RESPONSE:
        return;
		str.Format(_T("Receiving response..."));
		break;

	case INTERNET_STATUS_RESPONSE_RECEIVED:
		str.Format(_T("Response received"));
		break;

	case INTERNET_STATUS_CLOSING_CONNECTION:
		str.Format(_T("Closing the connection to the server"));
		break;

	case INTERNET_STATUS_CONNECTION_CLOSED:
		str.Format(_T("Connection to the server closed"));
		break;

	case INTERNET_STATUS_HANDLE_CLOSING:
        return;
		str.Format(_T("Handle closed"));
		break;

	case INTERNET_STATUS_REQUEST_COMPLETE:
        // See the CInternetSession constructor for details on INTERNET_FLAG_ASYNC.
        // The lpvStatusInformation parameter points at an INTERNET_ASYNC_RESULT 
        // structure, and dwStatusInformationLength contains the final completion 
        // status of the asynchronous function. If this is ERROR_INTERNET_EXTENDED_ERROR, 
        // the application can retrieve the server error information by using the 
        // Win32 function InternetGetLastResponseInfo. See the ActiveX SDK for more 
        // information about this function. 
		if (dwStatusInformationLength == sizeof(INTERNET_ASYNC_RESULT))
		{
			INTERNET_ASYNC_RESULT* pResult = (INTERNET_ASYNC_RESULT*) lpvStatusInformation;
			str.Format(_T("Request complete, dwResult = %8.8X, dwError = %8.8X"), pResult->dwResult, pResult->dwError);
		}
		else
			str.Format(_T("Request complete"));
		break;

	case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
	case INTERNET_STATUS_REDIRECT:
	default:
		str.Format(_T("Unknown status: %d"), dwInternetStatus);
		break;
	}

    SetStatus(str);

    //TRACE("CWebGrabSession::OnStatusCallback: %s\n",str);
}

void CWebGrabSession::SetStatus(LPCTSTR fmt, ...)
{
    va_list args;
    TCHAR buffer[512];

    va_start(args, fmt);
    _vstprintf_s(buffer,512, fmt, args);
    va_end(args);

    //TRACE1("CWebGrabSession::SetStatus: %s\n", buffer);
	if(m_pStatusWnd){
		if (IsWindow(m_pStatusWnd->m_hWnd)){
			m_pStatusWnd->SetWindowText(buffer);
			m_pStatusWnd->RedrawWindow();
		}
	}
}

