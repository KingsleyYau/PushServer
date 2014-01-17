// PushServerStressTest_Client.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "PushServerStressTest_Client.h"
#include "PushServerStressTest_ClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPushServerStressTest_ClientApp

BEGIN_MESSAGE_MAP(CPushServerStressTest_ClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPushServerStressTest_ClientApp ����

CPushServerStressTest_ClientApp::CPushServerStressTest_ClientApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CPushServerStressTest_ClientApp ����

CPushServerStressTest_ClientApp theApp;


// CPushServerStressTest_ClientApp ��ʼ��

BOOL CPushServerStressTest_ClientApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	// �������ݿ�
	TCHAR szDBFilePath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szDBFilePath, MAX_PATH);
	PathRemoveFileSpec(szDBFilePath);
	_tcscat_s(szDBFilePath, _T("\\PushMsg.db"));
	ClientDB::GetInstance()->OpenDB(szDBFilePath, NULL);
//	ClientDB::GetInstance()->ExecuteSQL(TEXT("BEGIN;"));

	CPushServerStressTest_ClientDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˴����ô����ʱ�á�ȷ�������ر�
		//  �Ի���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ�á�ȡ�������ر�
		//  �Ի���Ĵ���
	}
//	ClientDB::GetInstance()->ExecuteSQL(TEXT("COMMIT;"));
	ClientDB::GetInstance()->CloseDB();


	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}
