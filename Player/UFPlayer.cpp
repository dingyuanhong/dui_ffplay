
#include "UFPlayer.h"
#include "Shellapi.h"

#include "FileDialog.h"

namespace DuiLib
{
void PPosCallback(void *usr,int64_t pos,int64_t max)
{
	UFPlayer *pthis = (UFPlayer*)usr;
	pthis->SetMedioPos(pos,max);
}

UFPlayer::UFPlayer(void)
	:m_pTitle(NULL),m_pOPENFILE(NULL),
	m_pSPEEDDOWN(NULL),
	m_pPLAYSEEK(NULL),m_pSPEEDUP(NULL),
	m_pPTS(NULL),m_pSTOP(NULL),
	m_pPRE(NULL),m_pPLAY(NULL),
	m_pPAUSE(NULL),m_pNEXT(NULL),
	m_pVOLUME(NULL),m_pMUTE(NULL),
	m_pUNMUTE(NULL),m_bMute(0),
	m_pVideo(NULL),m_player(NULL),
	m_bChangeFile(false)
{
}

UFPlayer::~UFPlayer(void)
{
}

DuiLib::CDuiString UFPlayer::GetSkinFolder()
{
	return CPaintManagerUI::GetResourcePath();
}
DuiLib::CDuiString UFPlayer::GetSkinFile()
{
	return _T("UFPlayer.xml");
}
LPCTSTR UFPlayer::GetWindowClassName(void) const
{
	return _T("UFPlayer");
}
LRESULT UFPlayer::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch( uMsg ) {
		case WM_SYSCOMMAND:
			if((SC_MAXIMIZE & wParam)==SC_MAXIMIZE || (SC_RESTORE & wParam) == SC_RESTORE){
				LRESULT lRet = UFWindow::HandleMessage(uMsg, wParam, lParam);
				if(m_player != NULL){
					if(m_pVideo != NULL){
						int x = m_pVideo->GetWidth();
						int y = m_pVideo->GetHeight();
						m_player->Resize(x,y);
						m_pVideo->MoveWindow(0,30);
					}
				}
				return lRet;
			}
			break;
	}
	return UFWindow::HandleMessage(uMsg, wParam, lParam);
}
LRESULT UFPlayer::ResponseDefaultKeyEvent(WPARAM wParam)
{
	if (wParam == VK_RETURN)
	{
		return FALSE;
	}
	else if (wParam == VK_ESCAPE)
	{
		Close();
		return TRUE;
	}

	return FALSE;
}
LRESULT UFPlayer::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	PostQuitMessage(1);
	return UFWindow::OnClose(uMsg,wParam,lParam,bHandled);
}
void UFPlayer::Notify(TNotifyUI& msg)
{
	CDuiString strName = msg.pSender->GetName();
	if(msg.sType == DUI_MSGTYPE_SETFOCUS)
	{

	}else if(msg.sType == DUI_MSGTYPE_KILLFOCUS)
	{
	
	}else
	if(msg.sType == DUI_MSGTYPE_VALUECHANGED)
	{
		if(msg.pSender == m_pPLAYSEEK)
		{
			int nPos = m_pPLAYSEEK->GetValue();
			if(m_player != NULL) m_player->SetPos(nPos);
		}
		else if(msg.pSender == m_pVOLUME)
		{
			int nValue = m_pVOLUME->GetValue();
			if(m_player != NULL){
				AudioRender *audio = m_player->GetAudioRender();
				if(audio != NULL){
					audio->SetVolume(nValue);
				}
			}
		}
	}
	else
	{
		UFWindow::Notify(msg);
	}
}
void UFPlayer::OnClick(TNotifyUI& msg)
{
	CDuiString strName = msg.pSender->GetName();
	if(strName == _T("IDCLOSE"))
	{
		Close();
	}
	else if(strName == _T("IDMINIMIZE"))
	{
		SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
	}
	else if(msg.pSender == m_pOPENFILE)
	{
		OnClickOpenFile();
	}
	else if(msg.pSender == m_pSPEEDDOWN)
	{
		OnClickSpeedDown();
	}
	else if(msg.pSender == m_pSPEEDUP)
	{
		OnClickSpeedUp();
	}
	else if(msg.pSender == m_pSTOP)
	{
		OnClickStop();
	}
	else if(msg.pSender == m_pPRE)
	{
		OnClickPre();
	}
	else if(msg.pSender == m_pPLAY)
	{
		OnClickPlay();
	}
	else if(msg.pSender == m_pPAUSE)
	{
		OnClickPause();
	}
	else if(msg.pSender == m_pNEXT)
	{
		OnClickNext();
	}
	else if(msg.pSender == m_pMUTE)
	{
		if(m_player != NULL){
			m_bMute = 1;
			AudioRender *audio = m_player->GetAudioRender();
			if(audio != NULL){
				audio->Mute(m_bMute);
			}
			m_pUNMUTE->SetVisible(true);
			m_pMUTE->SetVisible(false);
		}
	}
	else if(msg.pSender == m_pUNMUTE)
	{
		if(m_player != NULL){
			m_bMute = 0;
			AudioRender *audio = m_player->GetAudioRender();
			if(audio != NULL){
				audio->Mute(m_bMute);
			}
			m_pUNMUTE->SetVisible(false);
			m_pMUTE->SetVisible(true);
		}
	}
	else{
		UFWindow::OnClick(msg);
	}
}
LRESULT UFPlayer::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lRet = UFWindow::OnSize(uMsg,wParam,lParam,bHandled);
	if(m_pVideo != NULL){
		m_pVideo->MoveWindow(0,30);
		m_pVideo->SetHeight(GetHeight() - 30 - 60);
		m_pVideo->SetWidth(GetWidth());
	}

	return lRet;
}
void UFPlayer::InitWindow()
{
	UFWindow::InitWindow();
	m_pTitle = (CLabelUI*)m_PaintManager.FindControl(_T("IDTITLE"));
	m_pOPENFILE = (CButtonUI*)m_PaintManager.FindControl(_T("IDOPENFILE"));
	m_pSPEEDDOWN = (CButtonUI*)m_PaintManager.FindControl(_T("IDSPEEDDOWN"));
	m_pPLAYSEEK = (CSliderUI*)m_PaintManager.FindControl(_T("IDPLAYSEEK"));
	m_pSPEEDUP = (CButtonUI*)m_PaintManager.FindControl(_T("IDSPEEDUP"));
	m_pPTS = (CLabelUI*)m_PaintManager.FindControl(_T("IDPTS"));
	m_pSTOP = (CButtonUI*)m_PaintManager.FindControl(_T("IDSTOP"));
	m_pPRE = (CButtonUI*)m_PaintManager.FindControl(_T("IDPRE"));
	m_pPLAY = (CButtonUI*)m_PaintManager.FindControl(_T("IDPLAY"));
	m_pPAUSE = (CButtonUI*)m_PaintManager.FindControl(_T("IDPAUSE"));
	m_pNEXT = (CButtonUI*)m_PaintManager.FindControl(_T("IDNEXT"));
	m_pVOLUME = (CSliderUI*)m_PaintManager.FindControl(_T("IDVOLUME"));
	m_pMUTE = (CButtonUI*)m_PaintManager.FindControl(_T("IDMUTE"));
	m_pUNMUTE = (CButtonUI*)m_PaintManager.FindControl(_T("IDUNMUTE"));

	m_pVideo = new UFVideo();
	m_pVideo->Create(GetHWND(),_T(""),UI_WNDSTYLE_CHILD, WS_EX_STATICEDGE | WS_EX_APPWINDOW);
	m_pVideo->ShowWindow(false);
	m_pVideo->MoveWindow(0,30);
	m_pVideo->SetHeight(GetHeight() - 30 - 60);
	m_pVideo->SetWidth(GetWidth());


	//m_strFileName = _T("http:////play.68mtv.com:8080//play13//60622.mp4");
	int nValue = SDLGetMaxVolume();
	m_pVOLUME->SetMaxValue(nValue);
	m_pVOLUME->SetValue(nValue);
}

void UFPlayer::OnClickOpenFile()
{
	//文件过滤字符串。够长
	TCHAR * strfilter = _T("Common media formats\0*.avi;*.wmv;*.wmp;*.wm;*.asf;*.rm;*.ram;*.rmvb;*.ra;*.mpg;*.mpeg;*.mpe;*.m1v;*.m2v;*.mpv2;")
		_T("*.mp2v;*.dat;*.mp4;*.m4v;*.m4p;*.vob;*.ac3;*.dts;*.mov;*.qt;*.mr;*.3gp;*.3gpp;*.3g2;*.3gp2;*.swf;*.ogg;*.wma;*.wav;")
		_T("*.mid;*.midi;*.mpa;*.mp2;*.mp3;*.m1a;*.m2a;*.m4a;*.aac;*.mkv;*.ogm;*.m4b;*.tp;*.ts;*.tpr;*.pva;*.pss;*.wv;*.m2ts;*.evo;")
		_T("*.rpm;*.realpix;*.rt;*.smi;*.smil;*.scm;*.aif;*.aiff;*.aifc;*.amr;*.amv;*.au;*.acc;*.dsa;*.dsm;*.dsv;*.dss;*.pmp;*.smk;*.flic\0")
		_T("Windows Media Video(*.avi;*wmv;*wmp;*wm;*asf)\0*.avi;*.wmv;*.wmp;*.wm;*.asf\0")
		_T("Windows Media Audio(*.wma;*wav;*aif;*aifc;*aiff;*mid;*midi;*rmi)\0*.wma;*.wav;*.aif;*.aifc;*.aiff;*.mid;*.midi;*.rmi\0")
		_T("Real(*.rm;*ram;*rmvb;*rpm;*ra;*rt;*rp;*smi;*smil;*.scm)\0*.rm;*.ram;*.rmvb;*.rpm;*.ra;*.rt;*.rp;*.smi;*.smil;*.scm\0")
		_T("MPEG Video(*.mpg;*mpeg;*mpe;*m1v;*m2v;*mpv2;*mp2v;*dat;*mp4;*m4v;*m4p;*m4b;*ts;*tp;*tpr;*pva;*pss;*.wv;)\0")
		_T("*.mpg;*.mpeg;*.mpe;*.m1v;*.m2v;*.mpv2;*.mp2v;*.dat;*.mp4;*.m4v;*.m4p;*.m4b;*.ts;*.tp;*.tpr;*.pva;*.pss;*.wv;\0")
		_T("MPEG Audio(*.mpa;*mp2;*m1a;*m2a;*m4a;*aac;*.m2ts;*.evo)\0*.mpa;*.mp2;*.m1a;*.m2a;*.m4a;*.aac;*.m2ts;*.evo\0")
		_T("DVD(*.vob;*ifo;*ac3;*dts)\0*.vob;*.ifo;*.ac3;*.dts\0MP3(*.mp3)\0*.mp3\0CD Tracks(*.cda)\0*.cda\0")
		_T("Quicktime(*.mov;*qt;*mr;*3gp;*3gpp;*3g2;*3gp2)\0*.mov;*.qt;*.mr;*.3gp;*.3gpp;*.3g2;*.3gp2\0")
		_T("Flash Files(*.flv;*swf;*.f4v)\0*.flv;*.swf;*.f4v\0Playlist(*.smpl;*.asx;*m3u;*pls;*wvx;*wax;*wmx;*mpcpl)\0*.smpl;*.asx;*.m3u;*.pls;*.wvx;*.wax;*.wmx;*.mpcpl\0")
		_T("Others(*.ivf;*au;*snd;*ogm;*ogg;*fli;*flc;*flic;*d2v;*mkv;*pmp;*mka;*smk;*bik;*ratdvd;*roq;*drc;*dsm;*dsv;*dsa;*dss;*mpc;*divx;*vp6;*.ape;*.flac;*.tta;*.csf)")
		_T("\0*.ivf;*.au;*.snd;*.ogm;*.ogg;*.fli;*.flc;*.flic;*.d2v;*.mkv;*.pmp;*.mka;*.smk;*.bik;*.ratdvd;*.roq;*.drc;*.dsm;*.dsv;*.dsa;*.dss;*.mpc;*.divx;*.vp6;*.ape;*.amr;*.flac;*.tta;*.csf\0")
		_T("All Files(*.*)\0*.*\0\0");

	TCHAR szFileName[MAX_PATH];
	int nRet = DialogGetFileName(GetHWND(),strfilter,szFileName,MAX_PATH*sizeof(TCHAR));
	if(nRet == 1){
		if(m_strFileName != szFileName){
			m_bChangeFile = 1;
		}
		m_strFileName = szFileName;
	}
}
void UFPlayer::OnClickSpeedDown()
{
	if(m_player != NULL){
		m_player->AdjustPost(-60);
	}
}
void UFPlayer::OnClickSpeedUp()
{
	if(m_player != NULL){
		m_player->AdjustPost(60);
	}
}
void UFPlayer::OnClickStop()
{
	if(m_player != NULL)
	{
		m_player->Stop();
	}
	m_pPLAY->SetVisible(true);
	m_pPAUSE->SetVisible(false);
	m_pVideo->ShowWindow(false);
	m_pTitle->SetText(_T(""));
}
void UFPlayer::OnClickPre()
{
}
void UFPlayer::OnClickPlay()
{
	if(m_strFileName.IsEmpty()) return;
	if(m_player == NULL)
	{
		m_player = new MedioPlay();
		m_player->BindWindow(m_pVideo->GetHWND());
		m_player->SetPosCallback(PPosCallback,this);
	}
	CDuiString strName;
	int nIndex = m_strFileName.ReverseFind(_T('\\'));
	if(nIndex != -1){
		strName = m_strFileName.Right(m_strFileName.GetLength() - nIndex - 1);
	}
	m_pTitle->SetText(strName);
	std::string FileName;
#ifdef _UNICODE
	FileName = StrWTA(m_strFileName.GetData());
#else
	FileName = m_strFileName.GetData();
#endif
	if(m_bChangeFile){
		int nRet = m_player->Open(FileName.c_str());
		if(nRet == -1)
		{
			return;
		}
		m_player->Play();
	}else
	if(m_player->IsPause())
	{
		m_player->Play();
	}else if(m_player->IsPlaying())
	{
	}else if(m_player->IsStop())
	{
		m_player->Play();
	}else{
		int nRet = m_player->Open(FileName.c_str());
		if(nRet == -1)
		{
			return;
		}
		m_player->Play();
	}
	if(m_player != NULL){
		int nValue = m_pVOLUME->GetValue();
		AudioRender *audio = m_player->GetAudioRender();
		if(audio != NULL){
			audio->SetVolume(nValue);
			audio->Mute(m_bMute);
		}
	}
	m_pPLAY->SetVisible(false);
	m_pPAUSE->SetVisible(true);
	m_pVideo->ShowWindow(true);
	m_nCur = -1;
}
void UFPlayer::OnClickPause()
{
	if(m_player != NULL){
		if(m_player->IsPlaying())
		{
			m_player->Pause();
			
		}
		m_pPLAY->SetVisible(true);
		m_pPAUSE->SetVisible(false);
	}
}
void UFPlayer::OnClickNext()
{
}
void UFPlayer::SetMedioPos(int64_t pos,int64_t max)
{
	if(max > 0){
		m_pPLAYSEEK->SetMaxValue(max);
	}
	if(m_nCur == pos) return;
	m_nCur = pos;
	m_pPLAYSEEK->SetValue(pos);

	int nValue = pos;
	CDuiString strValue;
	strValue.Format(_T("%02d:%02d:%02d"),nValue/3600,(nValue/60)%60,(nValue%60));
	int nMax = m_pPLAYSEEK->GetMaxValue();
	CDuiString strMax;
	strMax.Format(_T("%02d:%02d:%02d"),nMax/3600,(nMax/60)%60,(nMax%60));
	strValue += _T("/");
	strValue += strMax;
	m_pPTS->SetText(strValue);
}
//void UFPlayer::OnDropFiles(HDROP hDropInfo)
//{
//	CDialogEx::OnDropFiles(hDropInfo);
//	LPTSTR pFilePathName =(LPTSTR)malloc(MAX_URL_LENGTH);
//	::DragQueryFile(hDropInfo, 0, pFilePathName,MAX_URL_LENGTH);  // 获取拖放文件的完整文件名，最关键！
//
//	m_inputurl.SetWindowText(pFilePathName);
//
//	::DragFinish(hDropInfo);   // 注意这个不能少，它用于释放Windows 为处理文件拖放而分配的内存
//	free(pFilePathName);
//}
}