
// ConsoleDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Console.h"
#include "ConsoleDlg.h"
#include "DlgAbout.h"

#include <atlwin.h>
#include "UI/UIHelper.h"
#include "UI/WZDialog.h"
#include "DlgCheckUser.h"
#include "LoginMgr.h"
#include "I8UIFrame/NonFrameChildDlg.h"
#include "DlgInfoMsg.h"
#include "DlgAlertInfo.h"
#include "MessageBox.h"
#include "DlgViewRunState.h"
#include "AsyncDataHelper.h"


#include "./ui/DlgMonitor.h"
#include "./ui/DlgServer.h"
#include "./ui/DlgClient.h"
#include "./ui/DlgDownLoad.h"
#include "./ui/DlgSyncTask.h"
#include "./ui/DlgPushGame.h"
#include "./ui/DlgStartup.h"
#include "./ui/DlgTools.h"
#include "./ui/DlgConfig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CConsoleDlg 对话框

enum { WM_UPDATE_MSG = WM_USER + 0x200, WM_CHECK_USER , WM_RECV_CONSOLEMSG, WM_ADD_EXCEPTION = WM_USER + 0x112, WM_SHOW_LOG };
enum { DT_TIMER = 10001 };


CConsoleDlg::CConsoleDlg(CWnd* pParent /*=NULL*/)
	: BaseClass(this, CConsoleDlg::IDD, pParent)
	, curRadio_(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CConsoleDlg::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_MAIN_SET, wndBtnSetting_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_MIN, wndBtnMin_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_MAX, wndBtnMax_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_CLOSE, wndBtnClose_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_HELP, wndBtnHelp_);
	DDX_Control(pDX, IDC_STATIC_CENTER_RES, wndCenterRes_);
	DDX_Control(pDX, IDC_STATIC_LOCAL_RES, wndLocalRes_);
	DDX_Control(pDX, IDC_STATIC_CLIENTS, wndClients_);
    DDX_Control(pDX, IDC_STATIC_SERVER_STATUS, wndServerStatus_);
    DDX_Control(pDX, IDC_STATIC_SELECT, wndSelectStatus_);
    DDX_Control(pDX, IDC_STATIC_STATUS, wndStatusImg_);

	DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_MONITOR, wndBtnMonitor_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_SERVER, wndBtnServer_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_CLIENT, wndBtnClient_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_RESOURCE, wndBtnResource_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_SYNCTASK, wndBtnSyncTask_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_PUSH, wndBtnPush_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_STARTUP, wndBtnStartup_);
	DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_TOOL, wndBtnTool_);
    DDX_Control(pDX, IDC_BUTTON_MAIN_TAB_CONFIG, wndBtnConfig_);
}


BEGIN_EASYSIZE_MAP(CConsoleDlg)   
	EASYSIZE(IDC_STATIC_MAIN_DIALOG, ES_BORDER, ES_BORDER, ES_BORDER, ES_BORDER, 0)
	EASYSIZE(IDC_STATIC_CENTER_RES, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0);
	EASYSIZE(IDC_STATIC_LOCAL_RES, IDC_STATIC_CENTER_RES, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0);
    EASYSIZE(IDC_STATIC_CLIENTS, IDC_STATIC_LOCAL_RES, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0);
    EASYSIZE(IDC_STATIC_SELECT, IDC_STATIC_CLIENTS, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0);
	EASYSIZE(IDC_STATIC_SERVER_STATUS, IDC_STATIC_SELECT, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0);
	EASYSIZE(IDC_STATIC_STATUS, IDC_STATIC_SELECT, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0);
END_EASYSIZE_MAP 


BEGIN_MESSAGE_MAP(CConsoleDlg, BaseClass)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_QUERYDRAGICON()

	ON_BN_CLICKED(IDC_BUTTON_MAIN_CLOSE, &CConsoleDlg::OnBnClickedButtonMainClose)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_MAX, &CConsoleDlg::OnBnClickedButtonMainMax)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_MIN, &CConsoleDlg::OnBnClickedButtonMainMin)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_SET, &CConsoleDlg::OnBnClickedButtonMainSet)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_HELP, &CConsoleDlg::OnBnClickedButtonHelp)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_MESSAGE(i8desk::ui::WM_MSG_REINIT, &CConsoleDlg::OnReInitMsg)
	ON_MESSAGE(i8desk::ui::WM_SELECT_TAB, &CConsoleDlg::OnSelectTab)
	ON_MESSAGE(WM_UPDATE_MSG, &CConsoleDlg::OnUpdateMsg)
	ON_MESSAGE(WM_CHECK_USER, &CConsoleDlg::OnCheckUserInfoMsg)
	ON_MESSAGE(WM_RECV_CONSOLEMSG, &CConsoleDlg::OnRecvConsoleMsg)
	ON_MESSAGE(i8desk::ui::WM_SAVE_HARDWARE_MSG, &CConsoleDlg::OnSaveHardwareMsg)
	ON_MESSAGE(WM_ADD_EXCEPTION, &CConsoleDlg::OnAddException)
	ON_MESSAGE(i8desk::ui::WM_SHOW_LOG, &CConsoleDlg::OnShowLog)
    ON_MESSAGE(i8desk::ui::WM_SHOW_LOG_MORE, &CConsoleDlg::OnShowLogMore)
    ON_MESSAGE(i8desk::ui::WM_UPDATE_SELECT_STATE, &CConsoleDlg::OnUpdateSelectState)
	ON_COMMAND(ID_HELP_VIDEO_HELP, &CConsoleDlg::OnVideoHelp)
	ON_COMMAND(ID_MONITOR_EXCEPTION, &CConsoleDlg::OnMonitorException)
	ON_COMMAND(ID_VIEW_RUN_STATE, &CConsoleDlg::OnViewRunState)
	ON_COMMAND(ID_HELP_FIX, &CConsoleDlg::OnFix)
	ON_COMMAND(ID_HELP_BBS, &CConsoleDlg::OnBBS)
	ON_COMMAND(ID_HELP_ONLINE, &CConsoleDlg::OnOnline)
	ON_COMMAND(ID_HELP_ABOUT, &CConsoleDlg::OnHelpAbout)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_MONITOR, &CConsoleDlg::OnBnClickedButtonMainTabMonitor)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_SERVER, &CConsoleDlg::OnBnClickedButtonMainTabServer)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_CLIENT, &CConsoleDlg::OnBnClickedButtonMainTabClient)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_RESOURCE, &CConsoleDlg::OnBnClickedButtonMainTabResource)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_SYNCTASK, &CConsoleDlg::OnBnClickedButtonMainTabSyncTask)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_PUSH, &CConsoleDlg::OnBnClickedButtonMainTabPush)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_STARTUP, &CConsoleDlg::OnBnClickedButtonMainTabStartup)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_TOOL, &CConsoleDlg::OnBnClickedButtonMainTabTool)
	ON_BN_CLICKED(IDC_BUTTON_MAIN_TAB_CONFIG, &CConsoleDlg::OnBnClickedButtonMainTabConfig)
END_MESSAGE_MAP()


void CConsoleDlg::_UpdateBtnRect()
{
	if( !wndBtnHelp_.GetSafeHwnd() )
		return;

	wndBtnMonitor_.Invalidate();
	wndBtnServer_.Invalidate();
	wndBtnClient_.Invalidate();
	wndBtnResource_.Invalidate();
	wndBtnPush_.Invalidate();
	wndBtnStartup_.Invalidate();
	wndBtnTool_.Invalidate();
	wndBtnConfig_.Invalidate();

	CRect rcClient;
	GetClientRect(&rcClient);

	CRect rc = rcClient;
	rc = CRect(rcClient.right - 210, 0, rcClient.right - 210 + 52, 19);
	wndBtnHelp_.MoveWindow(rc);

	rc.left = rc.right;
	rc.right = rc.left + 52;
	wndBtnSetting_.MoveWindow(rc);	

	rc.left = rc.right;
	rc.right = rc.left + 31;
	wndBtnMin_.MoveWindow(rc);

	rc.left = rc.right;
	rc.right = rc.left + 31;
	wndBtnMax_.MoveWindow(rc);
	

	rc.left = rc.right;
	rc.right = rc.left + 31;
	wndBtnClose_.MoveWindow(rc);
}

void CConsoleDlg::_UpdateStatics()
{
	staticcStatus_ = *(i8desk::GetRealDataMgr().GetStaticsStatus());

	if( !::IsWindow(GetSafeHwnd()) )
		return;
	PostMessage(WM_UPDATE_MSG);
}

void CConsoleDlg::_RecvConsoleMsg()
{
	PostMessage(WM_RECV_CONSOLEMSG);
}

void CConsoleDlg::_CheckUserNotify(bool suc)
{
	PostMessage(WM_CHECK_USER, suc);
}

// CConsoleDlg 消息处理程序

BOOL CConsoleDlg::OnInitDialog()
{
	BaseClass::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	SetWindowText(_T("i8desk控制台"));

	accel_.LoadAccelerators(IDR_ACCELERATOR);

	wndsMgr_.Register<CDlgMonitor>();
	wndsMgr_.Register<CDlgServer>();
	wndsMgr_.Register<CDlgClient>();
	wndsMgr_.Register<CDlgDownLoad>();
	wndsMgr_.Register<CDlgSyncTask>();
	wndsMgr_.Register<CDlgPushGame>();
	wndsMgr_.Register<CDlgStartup>();
	wndsMgr_.Register<CDlgTools>();
	wndsMgr_.Register<CDlgConfig>();


	INIT_EASYSIZE;
	
	controlBk_.Attach(i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/ControlBk.png")));
	topLogo_.Attach(i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_TopLogo.png")));
	statusLine_.Attach(i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_BtmBg_Line.png")));

	wndBtnHelp_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/HelpSetBtn_Default.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/HelpSetBtn_MoveOn.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/HelpSetBtn_Click.png")));
	wndBtnSetting_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/SetBtn_Default.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/SetBtn_MoveOn.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/SetBtn_Click.png")));
	wndBtnMin_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Mini_Default.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Mini_MoveOn.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Mini_Click.png")));
	wndBtnMax_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Max_Default.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Max_MoveOn.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Max_Click.png")));
	wndBtnClose_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Close_Default.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Close_MoveOn.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Close_Click.png")));

	wndBtnMonitor_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Monitor.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Monitor_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Monitor_Click.png")));
	wndBtnServer_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Server.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Server_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Server_Click.png")));
	wndBtnClient_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Client.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Client_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Client_Click.png")));
	wndBtnResource_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Resource.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Resource_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Resource_Click.png")));
	wndBtnSyncTask_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Sync_Normal.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Sync_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Sync_Click.png")));
	wndBtnPush_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Push.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Push_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Push_Click.png")));
	wndBtnStartup_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Startup.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Startup_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Startup_Click.png")));
	wndBtnTool_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Tool.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Tool_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Tool_Click.png")));
	wndBtnConfig_.SetImages(
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Config.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Config_Hover.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Navigate/Navigate_Config_Click.png")));

	// BK 
	HBITMAP topBk[] = 
	{
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_TopBg_Mid.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_TopBg_Left.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_TopBg_Right.png"))
	};
	for(size_t i = 0; i != _countof(topBk); ++i)
		topBmp_[i].Attach(topBk[i]);

	HBITMAP contentBK[] = 
	{
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_ContentBg_Left.png")),
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_ContentBg_Left.png")),
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_ContentBg_Right.png"))
	};
	for(size_t i = 0; i != _countof(topBk); ++i)
		contentBmp_[i].Attach(contentBK[i]);

	HBITMAP bottomBK[] = 
	{
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_BtmMidBg.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_BtmBg_Left.png")), 
		i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Server_BtmBg_Right.png"))
	};
	for(size_t i = 0; i != _countof(topBk); ++i)
		bottomBmp_[i].Attach(bottomBK[i]);


	// Status Bar
	wndCenterRes_.SetThemeParent(GetSafeHwnd());
	wndLocalRes_.SetThemeParent(GetSafeHwnd());
    wndClients_.SetThemeParent(GetSafeHwnd());
    wndSelectStatus_.SetThemeParent(GetSafeHwnd());
    wndServerStatus_.SetThemeParent(GetSafeHwnd());

	wndStatusImg_.SetImage(i8desk::ui::SkinMgrInstance().GetSkin(_T("Main/Ico_StatusOk.png")));
	
	MoveWindow(0, 0, 1024, 768 - 35);
	CenterWindow();

	wndsMgr_.SetWindow(this, GetDlgItem(IDC_STATIC_MAIN_DIALOG));

	// 主页为监控首页
	OnBnClickedButtonMainTabMonitor();
	wndBtnMonitor_.SetCheck(BST_CHECKED);

	// 注册统计数据回调
	i8desk::GetRealDataMgr().Register(std::tr1::bind(&CConsoleDlg::_UpdateStatics, this), MASK_PARSE_STATICS);

	// 注册控制台消息
	i8desk::GetRecvDataMgr().RegisterConsole(std::tr1::bind(&CConsoleDlg::_RecvConsoleMsg, this));

	// 注册用户检测回调
	i8desk::GetRecvDataMgr().RegisterCheckUser(std::tr1::bind(&CConsoleDlg::_CheckUserNotify, this, std::tr1::placeholders::_1));

	// 通知验证
	i8desk::GetControlMgr().CheckUser(_T(""), _T(""));

	// 根据中心指定，是否显示信息提示框
	bool shouTip = i8desk::GetDataMgr().GetOptVal(OPT_U_CONSHOWDIALOG, 0) == 1;
	if( shouTip )
	{
		CDlgInfoMsg dlg;
		dlg.DoModal();
	}

	// 创建运行状态监视
	runTypeState_.reset(new CDlgViewRunState);
	runTypeState_->Create(CDlgViewRunState::IDD);
	

	// 刷新状态栏
	SetTimer(DT_TIMER, 2000, 0);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}


BOOL CConsoleDlg::PreTranslateMessage(MSG *pMsg)
{
	if( accel_.TranslateAccelerator(GetSafeHwnd(), pMsg) )
		return TRUE;

	return __super::PreTranslateMessage(pMsg);
}

void CConsoleDlg::OnDestroy()
{
	runTypeState_->DestroyWindow();

	i8desk::GetRealDataMgr().UnRegister(MASK_PARSE_STATICS);
	wndsMgr_.UnRegisterAll();

	BaseClass::OnDestroy();
}


// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CConsoleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中d
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		//CPaintDC dc(this); // 用于绘制的设备上下文
		

		BaseClass::OnPaint();
	}
}

void CConsoleDlg::OnTimer(UINT_PTR)
{
	PostMessage(WM_UPDATE_MSG);
}

void CConsoleDlg::OnMonitorException()
{
	wndsMgr_.Get(CDlgMonitor::IDD)->PostMessage(i8desk::ui::WM_CHANGE_DEBUG_MODE);
}

void CConsoleDlg::OnViewRunState()
{
	runTypeState_->ShowWindow(SW_SHOW);
	runTypeState_->CenterWindow();
}


void CConsoleDlg::OnVideoHelp()
{
	stdex::tString videoUrl = _T("http://www.i8desk.com/view.html");
	::ShellExecute(GetSafeHwnd(), _T("open"), videoUrl.c_str(), _T(""), _T(""), SW_SHOWNORMAL);
}

void CConsoleDlg::OnFix()
{
	const stdex::tString NID = i8desk::GetDataMgr().GetOptVal(OPT_U_NID, _T(""));
	if( NID.empty() ) 
	{
		CMessageBox box(_T("提示"), _T("用户ID无效"));
		box.DoModal();
		return;
	}

	stdex::tString videoUrl = _T("http://desk.i8cn.com/ComplainManage/SMSRepair.aspx?AgentID=") + NID;
	::ShellExecute(GetSafeHwnd(), _T("open"), videoUrl.c_str(), _T(""), _T(""), SW_SHOWNORMAL);
}
void CConsoleDlg::OnBBS()
{
	stdex::tString videoUrl = _T("http://bbs.i8desk.com");
	::ShellExecute(GetSafeHwnd(), _T("open"), videoUrl.c_str(), _T(""), _T(""), SW_SHOWNORMAL);
}

void CConsoleDlg::OnOnline()
{
	stdex::tString videoUrl = _T("http://chat10.live800.com/live800/chatClient/chatbox.jsp?companyID=79067&configID=115522&jid=2364604739&enterurl=http%3A%2F%2Fwww%2Ei8cn%2Ecom%2F&timestamp=1240819035078");
	::ShellExecute(GetSafeHwnd(), _T("open"), videoUrl.c_str(), _T(""), _T(""), SW_SHOWNORMAL);
}

void CConsoleDlg::OnHelpAbout()
{
	CDlgAbout dlg;
	dlg.DoModal();
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CConsoleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CConsoleDlg::OnSize(UINT nType, int cx, int cy)
{
	BaseClass::OnSize(nType, cx, cy);

	UPDATE_EASYSIZE;

	_UpdateBtnRect();

	wndsMgr_.UpdateSize();
}

void CConsoleDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	BaseClass::OnSizing(fwSide, pRect);

	EASYSIZE_MINSIZE(500, 400, fwSide, pRect);
}



LRESULT CConsoleDlg::OnUpdateMsg(WPARAM wParam, LPARAM lParam)
{
	stdex::tOstringstream os;

	stdex::tString IdcSize;
	i8desk::ulonglong tmp = staticcStatus_.IdcSize ;
	i8desk::FormatSize(tmp, IdcSize);
	os << _T("中心资源: ") << staticcStatus_.IdcCount << _T("个/") << IdcSize;
	wndCenterRes_.SetWindowText(os.str().c_str());
	os.str(_T(""));

	stdex::tString localSize;
	tmp = staticcStatus_.LocalSize ;
    i8desk::FormatSize(tmp, localSize);
    os << _T("本地资源: ") << staticcStatus_.LocalCount << _T("个/") << localSize;
    wndLocalRes_.SetWindowText(os.str().c_str());
	os.str(_T(""));

	os << _T("客户机: ") << staticcStatus_.OnlineClients << _T("/") << staticcStatus_.TotalClients;
	wndClients_.SetWindowText(os.str().c_str());

    //选择状态
    wndSelectStatus_.SetWindowText(selectState_.GetText().c_str());

    // 连接服务器状态
	LPCTSTR imgStatus = i8desk::GetManagerInstance().IsConnect() ? _T("Main/Ico_StatusOk.png") : _T("Main/Ico_StatusOk.png");
	wndStatusImg_.SetImage(i8desk::ui::SkinMgrInstance().GetSkin(imgStatus));

	LPCTSTR status = i8desk::GetManagerInstance().IsConnect() ? _T("成功") : _T("失败");
	stdex::tString text = _T("连接服务器[");
	text += CA2T(i8desk::loginHelper.ip_.c_str()) + _T("]") + status;
	wndServerStatus_.SetWindowText(text.c_str());
	return TRUE;
}

LRESULT CConsoleDlg::OnReInitMsg(WPARAM wParam, LPARAM lParam)
{
	wndsMgr_.ForEach(std::tr1::bind(&CWnd::PostMessage, std::tr1::placeholders::_1, i8desk::ui::WM_MSG_REINIT, 0, 0));

	return TRUE;
}

LRESULT CConsoleDlg::OnSelectTab(WPARAM wParam, LPARAM lParam)
{
	struct
	{
		UINT id;
		CButton &btn_;
	}Info[] = 
	{
		{ CDlgMonitor::IDD, wndBtnMonitor_ },
		{ CDlgServer::IDD, wndBtnServer_ },
		{ CDlgClient::IDD, wndBtnClient_ },
		{ CDlgDownLoad::IDD, wndBtnResource_ },
		{ CDlgSyncTask::IDD, wndBtnSyncTask_ },
		{ CDlgPushGame::IDD, wndBtnPush_ },
		{ CDlgStartup::IDD, wndBtnStartup_ },
		{ CDlgTools::IDD, wndBtnTool_ },
		{ CDlgConfig::IDD, wndBtnConfig_ }
	};

	wndsMgr_.Select(Info[wParam].id);
	wndsMgr_.Get(Info[wParam].id)->PostMessage(i8desk::ui::WM_APPLY_SELECT, lParam);

	curRadio_->SetCheck(BST_UNCHECKED);
	curRadio_ = &(Info[wParam].btn_);
	curRadio_->SetCheck(BST_CHECKED);

	return TRUE;
}

BOOL CConsoleDlg::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	UI::CMemDC pMemDC(pDC, &rcClient);
	pMemDC.SetBkMode(TRANSPARENT);
	
	// draw control bg
	CRect rcControlBg(0, 28, rcClient.right, 28 + controlBk_.GetHeight() + 3);
	CRect rcImgBk(0, 0, controlBk_.GetWidth(), controlBk_.GetHeight());
	controlBk_.Draw(pMemDC.GetSafeHdc(), rcControlBg, rcImgBk);
	//draw title bar.
	long nTop = 0;
	CRect rx(0, nTop, rcClient.right, nTop+28);

	UI::AfxUIDrawImage(&pMemDC, rx, &topBmp_[0], &topBmp_[1], &topBmp_[2]);


	topLogo_.Draw(pMemDC.GetSafeHdc(), 6, nTop);

	nTop += rx.Height();

	//draw menu bar.
	rx = CRect(0, nTop, rx.right, nTop+58);
	nTop += rx.Height();

	//draw content area.
	rx = CRect(0, nTop, rcClient.right, rcClient.bottom -26);
	UI::AfxUIDrawImage(&pMemDC, rx, 0, &contentBmp_[1], &contentBmp_[2]);

	//draw status bar.
	nTop = rcClient.bottom - 26;
	rx = CRect(0, nTop, rx.right, rcClient.bottom);
	UI::AfxUIDrawImage(&pMemDC, rx, &bottomBmp_[0], &bottomBmp_[1], &bottomBmp_[2]);


	statusLine_.Draw(pMemDC.GetSafeHdc(), 170, nTop);
	statusLine_.Draw(pMemDC.GetSafeHdc(), 370, nTop);
	statusLine_.Draw(pMemDC.GetSafeHdc(), 500, nTop);
	statusLine_.Draw(pMemDC.GetSafeHdc(), 660, nTop);

	return TRUE;
}


void CConsoleDlg::OnBnClickedButtonMainClose()
{
	EndTopDialog();

	if( ::IsWindow(GetSafeHwnd()) )
		PostQuitMessage(0);
}

void CConsoleDlg::OnBnClickedButtonMainMax()
{
	BaseClass::MaxOrRestore();
}

void CConsoleDlg::OnBnClickedButtonMainMin()
{
	ShowWindow(SW_MINIMIZE);
}

void CConsoleDlg::OnBnClickedButtonMainSet()
{
	CWZDialog dlg;
	if( IDOK == dlg.DoModal() )
		SendMessage(i8desk::ui::WM_MSG_REINIT);
}

void CConsoleDlg::OnBnClickedButtonHelp()
{
	CMenu menu;
	menu.LoadMenu(IDR_MENU_HELP);
	CMenu *subMenu = menu.GetSubMenu(0);

	CRect rc;
	wndBtnHelp_.GetWindowRect(rc);

	subMenu->TrackPopupMenu(TPM_LEFTALIGN, rc.left, rc.bottom, this);
}

LRESULT CConsoleDlg::OnCheckUserInfoMsg(WPARAM wParam, LPARAM lParam)
{
	// 弹窗
	if( wParam )
	{
		AlertDlg();
	}

	{
		static bool show = true; 
		if( wParam )
			return 0;

		if( show )
		{
			show = false;
			// 验证
			CDlgCheckUser dlg;
			if( IDOK != dlg.DoModal() )
			{
				PostQuitMessage(0);
			}

			show = true;
		}
	}
	

	return 1;
}

LRESULT CConsoleDlg::OnRecvConsoleMsg(WPARAM wParam, LPARAM lParam)
{
	{
		CMessageBox msgDlg(_T("提示"),_T("已经有另外控制台登录"));
		msgDlg.DoModal();
	}

	OnBnClickedButtonMainClose();

	return 1;
}

LRESULT CConsoleDlg::OnSaveHardwareMsg(WPARAM, LPARAM)
{
	wndsMgr_.Get(CDlgMonitor::IDD)->PostMessage(i8desk::ui::WM_SAVE_HARDWARE_MSG);
	return 1;
}

LRESULT CConsoleDlg::OnAddException(WPARAM ,LPARAM)
{
	wndsMgr_.Get(CDlgMonitor::IDD)->PostMessage(WM_ADD_EXCEPTION);
	return 1;
}

LRESULT CConsoleDlg::OnShowLog(WPARAM ,LPARAM)
{
	runTypeState_->ShowWindow(SW_SHOW);
	runTypeState_->CenterWindow();
	return 1;
}

LRESULT CConsoleDlg::OnShowLogMore(WPARAM ,LPARAM)
{
	wndsMgr_.Get(CDlgMonitor::IDD)->PostMessage(i8desk::ui::WM_CHANGE_DEBUG_MODE);
	return 1;
}

LRESULT CConsoleDlg::OnUpdateSelectState(WPARAM wParam,LPARAM lParam)
{
    if (wParam == 0)
    {
        if (selectState_.Set(lParam))
        {
            wndSelectStatus_.SetWindowText(selectState_.GetText().c_str());
        }
    }
    else
    {
        const stdex::tString* psInfoEx = reinterpret_cast<const stdex::tString*>(wParam);
        if (selectState_.Set(lParam, *psInfoEx))
        {
            wndSelectStatus_.SetWindowText(selectState_.GetText().c_str());
        }
    }
    return 1;
}

void CConsoleDlg::OnBnClickedButtonMainTabMonitor()
{
	wndsMgr_.Select(CDlgMonitor::IDD);
	curRadio_ = &wndBtnMonitor_;
}

void CConsoleDlg::OnBnClickedButtonMainTabServer()
{
	wndsMgr_.Select(CDlgServer::IDD);
	curRadio_ = &wndBtnServer_;
}

void CConsoleDlg::OnBnClickedButtonMainTabClient()
{
	wndsMgr_.Select(CDlgClient::IDD);
	curRadio_ = &wndBtnClient_;
}

void CConsoleDlg::OnBnClickedButtonMainTabResource()
{
	wndsMgr_.Select(CDlgDownLoad::IDD);
	curRadio_ = &wndBtnResource_;
}

void CConsoleDlg::OnBnClickedButtonMainTabSyncTask()
{
	wndsMgr_.Select(CDlgSyncTask::IDD);
	curRadio_ = &wndBtnSyncTask_;
}



void CConsoleDlg::OnBnClickedButtonMainTabPush()
{
	wndsMgr_.Select(CDlgPushGame::IDD);
	curRadio_ = &wndBtnPush_;
}

void CConsoleDlg::OnBnClickedButtonMainTabStartup()
{
	wndsMgr_.Select(CDlgStartup::IDD);
	curRadio_ = &wndBtnStartup_;
}


void CConsoleDlg::OnBnClickedButtonMainTabTool()
{
	wndsMgr_.Select(CDlgTools::IDD);
	curRadio_ = &wndBtnTool_;
}

void CConsoleDlg::OnBnClickedButtonMainTabConfig()
{
	wndsMgr_.Select(CDlgConfig::IDD);
	curRadio_ = &wndBtnConfig_;
}
