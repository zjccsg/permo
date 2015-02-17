#include "stdafx.h"
#include "MenuEx.h"
#include "resource.h"

CMenuEx::CMenuEx()
{
}


CMenuEx::~CMenuEx()
{
}


void CMenuEx::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	//itemdata中只能放32位，一个图标就占满了，文字不能存放。因此采用定义结构体的方式
	//将结构体指针放在其中
	MENUITEM* pMi = (PMENUITEM)lpDrawItemStruct->itemData;
	ASSERT(pMi);
	//or CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	HDC hDC = lpDrawItemStruct->hDC;
	CRect rcItem = lpDrawItemStruct->rcItem;

	CDC dc;
	dc.Attach(hDC);
	int nBkMode = dc.SetBkMode(TRANSPARENT);

	// Save these value to restore them when done drawing.
	COLORREF crOldTextColor = dc.GetTextColor();
	COLORREF crOldBkColor = dc.GetBkColor();

	if ((lpDrawItemStruct->itemAction & ODA_SELECT) &&
		(lpDrawItemStruct->itemState  & ODS_SELECTED))
	{
		dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
		dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_HIGHLIGHT));
	}
	else
	{
		dc.FillSolidRect(&lpDrawItemStruct->rcItem, crOldBkColor);
	}
	CRect rcIcon = rcItem;
	rcIcon.right = rcItem.left + rcItem.Height();
	rcIcon.DeflateRect(2, 2, 2, 2);
	if (lpDrawItemStruct->itemState & ODS_CHECKED)
	{
		DrawIconEx(hDC, rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CHECK)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
	}
	else
	{
		DrawIconEx(hDC, rcIcon.left, rcIcon.top, pMi->hIcon, rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
	}

	CRect rcText = rcItem;
	rcText.left = rcItem.Height();

	dc.DrawText(pMi->strText, rcText, DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_LEFT);

	dc.SetBkMode(nBkMode); 
	dc.SetTextColor(crOldTextColor);
	dc.SetBkColor(crOldBkColor);

	dc.Detach();
}


void CMenuEx::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	lpMeasureItemStruct->itemHeight = 20;
	lpMeasureItemStruct->itemWidth = 80;
}
