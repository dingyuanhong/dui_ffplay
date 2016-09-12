#ifndef UFVIDEO_H
#define UFVIDEO_H


#include "UFWindow.h"

namespace DuiLib
{

class UFVideo
	: public UFWindow
{
public:
	UFVideo(void);
	~UFVideo(void);
protected:
	virtual CDuiString GetSkinFolder();
	virtual CDuiString GetSkinFile();
	virtual LPCTSTR GetWindowClassName(void) const;

	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	virtual void Notify(TNotifyUI& msg);
	virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam);
	virtual void OnClick(TNotifyUI& msg);
	virtual void InitWindow();
private:

};

}


#endif