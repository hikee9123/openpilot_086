#pragma once


// CWndOpenGL

class CWndOpenGL : public CWnd
{
	DECLARE_DYNAMIC(CWndOpenGL)

public:
	CWndOpenGL();
	virtual ~CWndOpenGL();


private:
	HGLRC m_hRC;

	double  m_dX, m_dY, m_dZ;
	double  m_dRX, m_dRY, m_dRZ;

	GLUquadricObj* m_pNewObj;

	int   m_nLButtonDown;
	CPoint  m_ptOld;

private:
	void   OnDraw();

	void   Draw3D();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()



public:

	

	
};


