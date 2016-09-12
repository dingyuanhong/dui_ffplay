#include "UFVideo.h"

namespace DuiLib
{

UFVideo::UFVideo(void)
{
}
UFVideo::~UFVideo(void)
{
}
CDuiString UFVideo::GetSkinFolder()
{
	return CPaintManagerUI::GetResourcePath();
}
CDuiString UFVideo::GetSkinFile()
{
	return _T("UFVideo.xml");
}
LPCTSTR UFVideo::GetWindowClassName(void) const
{
	return _T("UFVideo");
}

LRESULT UFVideo::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch( uMsg ) {
		case WM_SYSCOMMAND:
			if((SC_MAXIMIZE & wParam)==SC_MAXIMIZE) return 0;
			break;
		case WM_PAINT:
			return 0;
			break;
	}
	return UFWindow::HandleMessage(uMsg, wParam, lParam);
}
LRESULT UFVideo::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return UFWindow::OnClose(uMsg,wParam,lParam,bHandled);
}
void UFVideo::Notify(TNotifyUI& msg)
{
	CDuiString strName = msg.pSender->GetName();
	UFWindow::Notify(msg);
}
LRESULT UFVideo::ResponseDefaultKeyEvent(WPARAM wParam)
{
	if (wParam == VK_RETURN)
	{
		return FALSE;
	}
	else if (wParam == VK_ESCAPE)
	{
		return FALSE;
	}

	return FALSE;
}
void UFVideo::OnClick(TNotifyUI& msg)
{
	CDuiString strName = msg.pSender->GetName();
	UFWindow::OnClick(msg);
}
void UFVideo::InitWindow()
{
	UFWindow::InitWindow();
}

}