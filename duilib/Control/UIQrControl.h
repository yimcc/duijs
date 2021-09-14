#pragma once

namespace DuiLib {


class UILIB_API CQrControl:public CControlUI
{
public:
	CQrControl();
	~CQrControl();

	void SetText(LPCTSTR text);//�����ı�����
	virtual void PaintStatusImage(HDC hDC);//���ƶ�ά��
	virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);

	LPCTSTR GetClass() const { return _T("QrControl"); }

	//���ɶ�ά��ͼƬ
	static HBITMAP GenQrImage(HDC hdc,LPCTSTR text,int size);

	//���浽�ļ�
	bool SaveImage(LPCTSTR path); 
private:
	HBITMAP m_qrImage;//���ɵĶ�ά��ͼƬ
	CDuiString m_text;
};


}//duilib