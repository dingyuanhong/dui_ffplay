#ifndef UFPLAYER_H
#define UFPLAYER_H


#include "UFWindow.h"
#include "UFVideo.h"
#include "MedioPlay.h"
#include "Util.h"

namespace DuiLib
{

class UFPlayer
	: public UFWindow
{
public:
	UFPlayer(void);
	~UFPlayer(void);
	void SetMedioPos(int64_t pos,int64_t max);
private:
	void OnClickOpenFile();
	void OnClickSpeedDown();
	void OnClickSpeedUp();
	void OnClickStop();
	void OnClickPre();
	void OnClickPlay();
	void OnClickPause();
	void OnClickNext();
protected:
	virtual CDuiString GetSkinFolder();
	virtual CDuiString GetSkinFile();
	virtual LPCTSTR GetWindowClassName(void) const;

	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	virtual void Notify(TNotifyUI& msg);
	virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam);
	virtual void OnClick(TNotifyUI& msg);
	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual void InitWindow();
private:
	CLabelUI *m_pTitle;
	CButtonUI * m_pOPENFILE;
	CButtonUI * m_pSPEEDDOWN;
	CSliderUI * m_pPLAYSEEK;
	CButtonUI * m_pSPEEDUP;
	CLabelUI * m_pPTS;
	CButtonUI * m_pSTOP;
	CButtonUI * m_pPRE;
	CButtonUI * m_pPLAY;
	CButtonUI * m_pPAUSE;
	CButtonUI * m_pNEXT;
	CSliderUI * m_pVOLUME;
	CButtonUI * m_pMUTE;
	CButtonUI * m_pUNMUTE;
private:
	UFVideo *m_pVideo;
private:
	bool m_bChangeFile;
	CDuiString m_strFileName;
	MedioPlay *m_player;

	int m_nCur;
	int m_bMute;
};

};

#endif