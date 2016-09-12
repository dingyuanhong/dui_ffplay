#include "stdafx.h"
#include "UIFProgress.h"

namespace DuiLib
{
	LPCTSTR CFProgressUI::GetClass() const
	{
		return DUI_CTR_FPROGRESS;
	}

	LPVOID CFProgressUI::GetInterface(LPCTSTR pstrName)
	{
		if( _tcscmp(pstrName, DUI_CTR_FPROGRESS) == 0 ) return static_cast<CFProgressUI*>(this);
		return CProgressUI::GetInterface(pstrName);
	}
	void CFProgressUI::SetForegroundImage(LPCTSTR pStrImage)
	{
		if( m_diForeground.sDrawString == pStrImage && m_diForeground.pImageInfo != NULL ) return;
		m_diForeground.Clear();
		m_diForeground.sDrawString = pStrImage;
		Invalidate();
	}
	void CFProgressUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
	{
		if( _tcscmp(pstrName, _T("foregroundimage")) == 0 ) SetForegroundImage(pstrValue);
		else CProgressUI::SetAttribute(pstrName, pstrValue);
	}

	void CFProgressUI::PaintStatusImage(HDC hDC)
	{
		CProgressUI::PaintStatusImage(hDC);
		if( DrawImage(hDC, m_diForeground) ) return;
	}
}