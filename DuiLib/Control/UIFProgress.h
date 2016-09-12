#ifndef UIFPROGRESS_H
#define UIFPROGRESS_H

namespace DuiLib
{
	class DUILIB_API CFProgressUI : public CProgressUI
	{
	public:

		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);

		void SetForegroundImage(LPCTSTR pStrImage);
		void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
		void PaintStatusImage(HDC hDC);
	protected:
		TDrawInfo m_diForeground;
	};

} // namespace DuiLib

#endif