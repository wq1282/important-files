#pragma once




class CCustomGroup
	: public CStatic
{

private:
	ATL::CWindow parent_;
	WTL::CBrush backBrush_;
	ATL::CImage line_;
	WTL::CFontHandle font_;
	COLORREF textClr_;
	COLORREF bkClr_;
	UINT textStyle_;

public:
	CCustomGroup();
	virtual ~CCustomGroup();

public:
	virtual void PreSubclassWindow();

public:
	void SetThemeParent(HWND hWnd);
	void SetLine(HBITMAP bmp);
	void SetTextColor(COLORREF clr);
	void SetTextStyle(UINT style);
	void SetBkColor(COLORREF clr);

private:
	void _Init();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT);
	afx_msg BOOL OnEraseBkgnd(CDC*);
	afx_msg void OnMove(int, int);
	afx_msg void OnSize(UINT, int, int);
	afx_msg void OnSetText();
	afx_msg void OnPaint();
	afx_msg LRESULT OnPrintClient(WPARAM, LPARAM);
	afx_msg LRESULT OnSetFont(WPARAM, LPARAM);
	afx_msg LRESULT OnSetText(WPARAM, LPARAM);
};