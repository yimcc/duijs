#pragma once

class ImageLoader;

namespace DuiLib
{

//�첽����ͼƬ
class UILIB_API CImageControlUI : public CControlUI
{
	DECLARE_DUICONTROL(CImageControlUI)
public:
	enum FillType{
		kFill,
		kCenter,    //������ʾ
		kFitWidth,  //�������
		kFitHeight, //�߶�����
		kFitFull    //����
	};

	CImageControlUI();
	~CImageControlUI();
	LPCTSTR GetClass() const;
	LPVOID GetInterface(LPCTSTR pstrName);

	void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
	void PaintStatusImage(HDC hDC);

	void Load(LPCTSTR file);
protected:
	FillType m_fillType;
	HBITMAP  m_hBmp;
	ImageLoader* m_loader;
};

}

