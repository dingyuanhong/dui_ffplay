#pragma once
#ifndef UFWINDOW_H
#define UFWINDOW_H

#include <objbase.h>
#include <comdef.h>
#include <tchar.h>
#include "UIlib.h"

namespace DuiLib
{

class UFWindow
	:public WindowImplBase
{
public:
	UFWindow(void);
	~UFWindow(void);

	int GetWidth();
	int GetHeight();
	void SetWidth(int nWidth);
	void SetHeight(int nHeight);
	void MoveWindow(int x,int y);

	void Center();
	void FullWindow();
	bool IsShowWindow(){return m_bShowWindow;};
	void SetMovable(bool bMovable);

	virtual CControlUI* CreateControl(LPCTSTR pstrClass);
protected:
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	//关闭默认控件效果
	virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam){return FALSE;};
private:
	bool m_bShowWindow;

	bool m_bMovable;
	bool m_bLButtonDown;
	POINT m_LButtonDownPT;
};

};

#endif