#include "StdAfx.h"
#include "UFWindow.h"

namespace DuiLib
{

UFWindow::UFWindow(void)
:m_bShowWindow(false)
{
	m_bMovable = false;
	m_bLButtonDown = false;
	m_LButtonDownPT.x = 0;
	m_LButtonDownPT.y = 0;
}

UFWindow::~UFWindow(void)
{
}

int UFWindow::GetWidth()
{
	RECT childrc = {0};
	GetWindowRect(GetHWND(),&childrc);
	return childrc.right - childrc.left;
}
int UFWindow::GetHeight()
{
	RECT childrc = {0};
	GetWindowRect(GetHWND(),&childrc);
	return childrc.bottom - childrc.top;
}
void UFWindow::SetWidth(int nWidth)
{
	RECT childrc = {0};
	GetWindowRect(GetHWND(),&childrc);
	::MoveWindow(GetHWND(),childrc.left,childrc.top,nWidth,childrc.bottom - childrc.top,TRUE);
	SIZE sz = m_PaintManager.GetInitSize();
	m_PaintManager.SetInitSize(nWidth,sz.cy);
}
void UFWindow::SetHeight(int nHeight)
{
	RECT childrc = {0};
	GetWindowRect(GetHWND(),&childrc);
	::MoveWindow(GetHWND(),childrc.left,childrc.top,childrc.right - childrc.left,nHeight,TRUE);
	SIZE sz = m_PaintManager.GetInitSize();
	m_PaintManager.SetInitSize(sz.cx,nHeight);
}
void UFWindow::MoveWindow(int x,int y)
{
	::MoveWindow(GetHWND(),x,y,GetWidth(),GetHeight(),FALSE);
}
void UFWindow::Center()
{
	HWND hParent = ::GetParent(GetHWND());
	if(hParent == NULL){
		CenterWindow();
	}else{
		RECT rcDlg;
		GetWindowRect(GetHWND(),&rcDlg);
		RECT rcArea;
		::GetWindowRect(hParent, &rcArea);

		int DlgWidth = rcDlg.right - rcDlg.left;
		int DlgHeight = rcDlg.bottom - rcDlg.top;

		int xLeft = rcArea.left + (rcArea.right - rcArea.left)/2 - DlgWidth/2;
		int yTop = rcArea.top + (rcArea.bottom - rcArea.top)/2 - DlgHeight/2;
		::SetWindowPos(m_hWnd, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}
void UFWindow::FullWindow()
{
	RECT   rc;
	SystemParametersInfo(SPI_GETWORKAREA,0,(PVOID)&rc,0);
	SetWindowPos(GetHWND(),NULL,rc.left,rc.top,rc.right- rc.left,rc.bottom - rc.top,0);
}
void UFWindow::SetMovable(bool bMovable)
{
	m_bMovable = bMovable;
}
CControlUI* UFWindow::CreateControl(LPCTSTR pstrClass)
{
	return NULL;
}
LRESULT UFWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_SHOWWINDOW:
			{
				if(wParam == SW_HIDE){
					m_bShowWindow = false;
				}else{
					m_bShowWindow = true;
				}
			}
			break;
		case WM_DISPLAYCHANGE:
			m_PaintManager.SetInitSize((int)LOWORD(lParam),(int)HIWORD(lParam));
			//::MoveWindow(GetHWND(),0,0,(int)LOWORD(lParam),(int)HIWORD(lParam),TRUE);
			::PostMessage(GetHWND(),WM_SIZE,0,lParam);
			break;
		case WM_LBUTTONDOWN: OnLButtonDown(uMsg,wParam,lParam);break;
		case WM_LBUTTONUP: OnLButtonUp(uMsg,wParam,lParam);break;
		case WM_MOUSEMOVE: OnMouseMove(uMsg,wParam,lParam);break;
	}
	return WindowImplBase::HandleMessage(uMsg, wParam, lParam);
}

void UFWindow::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(m_bMovable == true){
		::GetCursorPos(&m_LButtonDownPT);
		POINT pt = m_LButtonDownPT;
		ScreenToClient(GetHWND(),&pt);
		CControlUI *pControl = m_PaintManager.FindControl(pt);
		if(pControl != NULL)
		{
			CDuiString strClass = pControl->GetClass();
			if(
				strClass == DUI_CTR_CONTROL || 
				strClass == DUI_CTR_VERTICALLAYOUT || 
				strClass == DUI_CTR_HORIZONTALLAYOUT
				)
			{
				m_bLButtonDown = true;
			}else{
				m_bLButtonDown = false;
			}
		}else{
			m_bLButtonDown = true;
		}
	}
}
void UFWindow::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(m_bMovable == true){
		m_LButtonDownPT.x = 0;
		m_LButtonDownPT.y = 0;
		m_bLButtonDown = false;
	}
}
void UFWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(m_bMovable == true){
		if(m_bLButtonDown && ( ::GetKeyState(VK_LBUTTON) < 0 )){
			bool bNeedMove = false;

			POINT oldPT;
			::GetCursorPos(&oldPT);
			if(!(oldPT.x == m_LButtonDownPT.x && oldPT.y == m_LButtonDownPT.y))
			{
				bNeedMove = true;
			}
			if(bNeedMove == true){
				POINT oldClientPt = m_LButtonDownPT;
				m_LButtonDownPT = oldPT;

				int nWidth = oldPT.x - oldClientPt.x;
				int nHeight = oldPT.y - oldClientPt.y;
			
				RECT rect = {0};
				GetWindowRect(GetHWND(),&rect);
				rect.left = rect.left + nWidth;
				rect.right = rect.right + nWidth;
				rect.top = rect.top + nHeight;
				rect.bottom = rect.bottom + nHeight;
				::SetWindowPos(m_hWnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOSIZE);
			}
		}
	}
}
}