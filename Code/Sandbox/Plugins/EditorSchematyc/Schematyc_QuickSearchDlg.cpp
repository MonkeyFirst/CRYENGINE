// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_QuickSearchDlg.h"

#include <Schematyc/Utils/Schematyc_StringUtils.h>

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	BEGIN_MESSAGE_MAP(CQuickSearchEditCtrl, CEdit)
		ON_WM_GETDLGCODE()
		ON_WM_KEYDOWN()
	END_MESSAGE_MAP()

	UINT CQuickSearchEditCtrl::OnGetDlgCode()
	{
		return (__super::OnGetDlgCode() | DLGC_WANTALLKEYS) & ~DLGC_HASSETSEL;
	}

	void CQuickSearchEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		switch(nChar)
		{
		case VK_LEFT:
		case VK_RIGHT:
		case VK_DELETE:
		case VK_HOME:
		case VK_END:
			{
				__super::OnKeyDown(nChar, nRepCnt, nFlags);
				break;
			}
		default:
			{
				if(CQuickSearchDlg* pParent = static_cast<CQuickSearchDlg*>(GetParent()))
				{
					pParent->OnKeyDown(nChar, nRepCnt, nFlags);
				}
				break;
			}
		}
	}

	BEGIN_MESSAGE_MAP(CQuickSearchDlg, CDialog)
		ON_WM_KEYDOWN()
		ON_EN_UPDATE(IDC_SCHEMATYC_QUICK_SEARCH_FILTER, OnFilterChanged)
		ON_NOTIFY(TVN_SELCHANGED, IDC_SCHEMATYC_QUICK_SEARCH_TREE, OnTreeSelChanged)
		ON_NOTIFY(NM_DBLCLK, IDC_SCHEMATYC_QUICK_SEARCH_TREE, OnTreeDblClk)
		ON_BN_CLICKED(IDC_SCHEMATYC_QUICK_SEARCH_VIEW_WIKI, OnViewWiki)
	END_MESSAGE_MAP()

	CQuickSearchDlg::CQuickSearchDlg(CWnd* pParent, CPoint pos, const IQuickSearchOptions& options, const char* szDefaultOption, const char* szDefaultFilter)
		: CDialog(IDD_SCHEMATYC_QUICK_SEARCH, pParent)
		, m_pos(pos)
		, m_options(options)
		, m_defaultOption(szDefaultOption)
		, m_defaultFilter(szDefaultFilter)
		, m_result(InvalidIdx)
	{}

	CQuickSearchDlg::~CQuickSearchDlg() {}

	const uint32 CQuickSearchDlg::GetResult() const
	{
		return m_result;
	}

	BOOL CQuickSearchDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		
		const char* szHeader = m_options.GetHeader();
		if(szHeader)
		{
			CDialog::SetWindowText(szHeader);
		}
		
		CDialog::OnInitDialog();

		if(!m_defaultFilter.empty())
		{
			m_filterCtrl.SetWindowText(m_defaultFilter.c_str());
			const int	filterLength = static_cast<int>(m_defaultFilter.length());
			m_filterCtrl.SetSel(filterLength, filterLength);
		}

		RefreshTreeCtrl(m_defaultOption.c_str());
		return true;
	}

	void CQuickSearchDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_QUICK_SEARCH_FILTER, m_filterCtrl);
		DDX_Control(pDX, IDC_SCHEMATYC_QUICK_SEARCH_TREE, m_treeCtrl);
		DDX_Control(pDX, IDC_SCHEMATYC_QUICK_SEARCH_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_SCHEMATYC_QUICK_SEARCH_VIEW_WIKI, m_viewWikiButton);
		DDX_Control(pDX, IDOK, m_okButton);
		CDialog::DoDataExchange(pDX);
	}

	void CQuickSearchDlg::OnOK()
	{
		if(HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem())
		{
			const uint32 selectedOptionIdx = static_cast<uint32>(m_treeCtrl.GetItemData(hSelectedItem));
			if(selectedOptionIdx < m_options.GetCount())
			{
				m_result = selectedOptionIdx;
				CDialog::OnOK();
			}
		}
	}

	void CQuickSearchDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		switch(nChar)
		{
			case VK_UP:
			{
				if(HTREEITEM hPrevItem = GetPrevTreeCtrlItem(m_treeCtrl.GetSelectedItem()))
				{
					m_treeCtrl.SelectItem(hPrevItem);
				}
				break;
			}
			case VK_DOWN:
			{
				if(HTREEITEM hNextItem = GetNextTreeCtrlItem(m_treeCtrl.GetSelectedItem(), false))
				{
					m_treeCtrl.SelectItem(hNextItem);
				}
				break;
			}
			case VK_RETURN:
			{
				if(m_okButton.IsWindowEnabled())
				{
					OnOK();
					EndDialog(TRUE);
				}
				break;
			}
			case VK_ESCAPE:
			{
				EndDialog(FALSE);
				break;
			}
			default:
			{
				__super::OnKeyDown(nChar, nRepCnt, nFlags);
				break;
			}
		}
	}

	void CQuickSearchDlg::OnFilterChanged()
	{
		RefreshTreeCtrl(nullptr);
		UpdateSelection();
	}

	void CQuickSearchDlg::OnTreeSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
	{
		UpdateSelection();
	}

	void CQuickSearchDlg::OnTreeDblClk(NMHDR* pNMHDR, LRESULT* pResult)
	{
		CPoint pos;
		GetCursorPos(&pos);
		m_treeCtrl.ScreenToClient(&pos);

		UINT      flags = 0;
		HTREEITEM hTreeItem = m_treeCtrl.HitTest(pos, &flags);
		if((flags & TVHT_ONITEM) && !m_treeCtrl.ItemHasChildren(hTreeItem))
		{
			OnOK();
			EndDialog(IDOK);
		}
	}

	void CQuickSearchDlg::OnViewWiki()
	{
		if(HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem())
		{
			const uint32 selectedOptionIdx = static_cast<uint32>(m_treeCtrl.GetItemData(hSelectedItem));
			if(selectedOptionIdx < m_options.GetCount())
			{
				PluginUtils::ViewWiki(m_options.GetWikiLink(selectedOptionIdx));
			}
		}
	}

	void CQuickSearchDlg::RefreshTreeCtrl(const char* szDefaultOption)
	{
		const uint32 optionCount = m_options.GetCount();
		uint32       prevOptionIdx = optionCount;
		HTREEITEM    hSelectedItem = m_treeCtrl.GetSelectedItem();
		if(hSelectedItem)
		{
			prevOptionIdx = static_cast<uint32>(m_treeCtrl.GetItemData(hSelectedItem));
		}

		m_treeCtrl.DeleteAllItems();

		CString filter;
		m_filterCtrl.GetWindowText(filter);

		bool bCollapseToRoot = filter.IsEmpty();
		for(uint32 optionIdx = 0; optionIdx < optionCount; ++ optionIdx)
		{
			const char* szOption = m_options.GetLabel(optionIdx);
			if(szOption && szOption[0])
			{
				if(filter.IsEmpty() || StringUtils::Filter(szOption, filter.GetString()))
				{
					std::vector<CStackString> tokens;
					const char*               szDelimiter = m_options.GetDelimiter();
					if(szDelimiter && szDelimiter[0])
					{
						CStackString tokenizedOption = szOption;
						const uint32 length = tokenizedOption.length();
						int          pos = 0;
						do
						{
							tokens.push_back(tokenizedOption.Tokenize(szDelimiter, pos));
						} while(pos < length);
					}
					else
					{
						tokens.push_back(szOption);
					}

					bool bDefaultOption = szDefaultOption && (strcmp(szOption, szDefaultOption) == 0);
					if(bDefaultOption)
					{
						bCollapseToRoot = false;
					}

					HTREEITEM hParentItem = NULL;
					for(uint32 tokenIdx = 0, tokenCount = tokens.size(); tokenIdx < tokenCount; ++ tokenIdx)
					{
						const char* szToken = tokens[tokenIdx].c_str();
						HTREEITEM   hItem = FindTreeCtrlItem(hParentItem, szToken, false);
						if(!hItem)
						{
							hItem = m_treeCtrl.InsertItem(szToken, 0, 0, hParentItem, TVI_LAST);
							if(!filter.IsEmpty())
							{
								m_treeCtrl.EnsureVisible(hItem);
							}
							if(tokenIdx == (tokenCount - 1))
							{
								if(!m_treeCtrl.GetSelectedItem() || (optionIdx == prevOptionIdx) || bDefaultOption)
								{
									m_treeCtrl.SelectItem(hItem);
								}
								m_treeCtrl.SetItemData(hItem, optionIdx);
							}
							else
							{
								m_treeCtrl.SetItemData(hItem, optionCount);
							}
						}
						hParentItem = hItem;
					}
				}
			}
		}

		// #SchematycTODO : Current approach sometimes results in items being expanded when they shouldn't.
		//                  A better approach might be to store the handle of the selected item when populating the tree, then expand and select afterwards.

		const HTREEITEM hFirstRootItem = m_treeCtrl.GetRootItem();
		if(bCollapseToRoot)
		{
			if(m_treeCtrl.GetNextSiblingItem(hFirstRootItem))
			{
				for(HTREEITEM hRootItem = hFirstRootItem; hRootItem; hRootItem = m_treeCtrl.GetNextSiblingItem(hRootItem))
				{
					m_treeCtrl.Expand(hRootItem, TVE_COLLAPSE);
				}
			}
			else
			{
				m_treeCtrl.Expand(hFirstRootItem, TVE_EXPAND);
			}
		}
		m_treeCtrl.EnsureVisible(hFirstRootItem);
	}

	void CQuickSearchDlg::UpdateSelection()
	{
		const char* szDescription = "";
		const char* szWikiLink = "";
		bool        bItemSelected = false;
		if(HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem())
		{
			const uint32 selectedOptionIdx = static_cast<uint32>(m_treeCtrl.GetItemData(hSelectedItem));
			if(selectedOptionIdx < m_options.GetCount())
			{
				szDescription = m_options.GetDescription(selectedOptionIdx);
				szWikiLink    = m_options.GetWikiLink(selectedOptionIdx);
				bItemSelected = true;
			}
		}
		m_descriptionCtrl.SetWindowText(szDescription);
		m_viewWikiButton.EnableWindow(szWikiLink && szWikiLink[0]);
		m_okButton.EnableWindow(bItemSelected);
	}

	HTREEITEM CQuickSearchDlg::FindTreeCtrlItem(HTREEITEM hRootItem, const char* name, bool recursive) const
	{
		CRY_ASSERT(name);
		if(name)
		{
			for(HTREEITEM hItem = m_treeCtrl.GetChildItem(hRootItem); hItem; hItem = m_treeCtrl.GetNextSiblingItem(hItem))
			{
				if(stricmp(m_treeCtrl.GetItemText(hItem), name) == 0)
				{
					return hItem;
				}

				if(recursive)
				{
					if(HTREEITEM hChildItem = FindTreeCtrlItem(hItem, name, true))
					{
						return hChildItem;
					}
				}
			}
		}
		return NULL;
	}

	HTREEITEM CQuickSearchDlg::GetPrevTreeCtrlItem(HTREEITEM hItem)
	{
		if(hItem)
		{
			if(HTREEITEM hPrevSiblingItem = m_treeCtrl.GetPrevSiblingItem(hItem))
			{
				return GetLastTreeCtrlItemChild(hPrevSiblingItem);
			}
			else if(HTREEITEM hParentItem = m_treeCtrl.GetParentItem(hItem))
			{
				return hParentItem;
			}
		}
		return NULL;
	}

	HTREEITEM CQuickSearchDlg::GetNextTreeCtrlItem(HTREEITEM hItem, bool ignoreChildren)
	{
		if(hItem)
		{
			if(!ignoreChildren)
			{
				if(HTREEITEM hChildItem = m_treeCtrl.GetChildItem(hItem))
				{
					return hChildItem;
				}
			}
			if(HTREEITEM hNextSiblingItem = m_treeCtrl.GetNextSiblingItem(hItem))
			{
				return hNextSiblingItem;
			}
			else
			{
				return GetNextTreeCtrlItem(m_treeCtrl.GetParentItem(hItem), true);
			}
		}
		return NULL;
	}

	HTREEITEM CQuickSearchDlg::GetLastTreeCtrlItemChild(HTREEITEM hItem)
	{
		HTREEITEM	hLastChildItem = hItem;
		if(hItem)
		{
			for(HTREEITEM hChildItem = m_treeCtrl.GetChildItem(hItem); hChildItem; hChildItem = m_treeCtrl.GetNextSiblingItem(hChildItem))
			{
				hLastChildItem = hChildItem;
			}
		}
		return hLastChildItem != hItem ? GetLastTreeCtrlItemChild(hLastChildItem) : hLastChildItem;
	}
}