#include <string.h>
#include <gccore.h>
#include "loader/fs.h"
#include "text.hpp"
#include "lockMutex.hpp"

#include "menu.hpp"
#include "http.h"
#include "sys.h"

#define GECKOURL "http://geckocodes.org/codes/R/%s.txt"

#define CHEATSPERPAGE 4

void CMenu::_hideCheatDownload(bool instant)
{
	m_btnMgr.hide(m_downloadBtnCancel, instant);
	m_btnMgr.hide(m_downloadPBar, instant);
	m_btnMgr.hide(m_downloadLblMessage[0], 0, 0, -2.f, 0.f, instant);
	m_btnMgr.hide(m_downloadLblMessage[1], 0, 0, -2.f, 0.f, instant);
}

void CMenu::_showCheatDownload(void)
{
	_setBg(m_downloadBg, m_downloadBg);
	m_btnMgr.show(m_downloadBtnCancel);
	m_btnMgr.show(m_downloadPBar);
}

u32 CMenu::_downloadCheatFileAsync(void *obj)
{
	CMenu *m = (CMenu *)obj;
	if (!m->m_thrdWorking)
		return 0;

	m->m_thrdStop = false;

	LWP_MutexLock(m->m_mutex);
	m->_setThrdMsg(m->_t("cfgg23", L"Downloading cheat file..."), 0);
	LWP_MutexUnlock(m->m_mutex);

	u32 bufferSize = 0x080000;	// Maximum download size 512kb
	SmartBuf buffer;
	block cheatfile;
	FILE *file;
	
	if (m->_initNetwork() < 0) {
		m->m_thrdWorking = false;
		return -1;
	}
	
	buffer = smartCoverAlloc(bufferSize);
	if (!buffer)
	{
		m->m_thrdWorking = false;
		return -2;
	}
	
	cheatfile = downloadfile(buffer.get(), bufferSize, sfmt(GECKOURL, m->m_cf.getId().c_str()).c_str(),CMenu::_downloadProgress, m);

	if (cheatfile.data != NULL && cheatfile.size > 65 && cheatfile.data[0] != '<') {
		// cheat file was downloaded (404's will now return emptybuffer)
		file = fopen(fmt("%s/%s.txt", m->m_txtCheatDir.c_str(), m->m_cf.getId().c_str()), "wb");
				
		if (file != NULL)
		{
			fwrite(cheatfile.data, 1, cheatfile.size, file);
			fclose(file);
			
			m->m_thrdWorking = false;
			return 0;
		}
	}
	
	m->m_thrdWorking = false;
	return -3;
}

void CMenu::_CheatSettings() 
{
	SetupInput();

	// try to load cheat file
	int txtavailable=0;
	m_cheatSettingsPage = 1; // init page
	
	txtavailable = m_cheatfile.openTxtfile(fmt("%s/%s.txt", m_txtCheatDir.c_str(), m_cf.getId().c_str())); 
	
	_showCheatSettings();
	_textCheatSettings();
	
	if (txtavailable)
		m_btnMgr.setText(m_cheatLblTitle,wfmt(L"%s",m_cheatfile.getGameName().c_str()));
	else 
		m_btnMgr.setText(m_cheatLblTitle,L"");
	
	while (true)
	{
		_mainLoopCommon();
		if (BTN_HOME_PRESSED || BTN_B_PRESSED)
			break;
		else if (BTN_UP_PRESSED)
			m_btnMgr.up();
		else if (BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if (BTN_MINUS_PRESSED || BTN_LEFT_PRESSED)
		{
			_hideCheatSettings();
			if (m_cheatSettingsPage == 1)
				m_cheatSettingsPage = (m_cheatfile.getCnt()+CHEATSPERPAGE-1)/CHEATSPERPAGE;
			else if (m_cheatSettingsPage > 1)
				--m_cheatSettingsPage;
			_showCheatSettings();

			m_btnMgr.click(m_wmote, m_cheatBtnPageM);
		}
		else if (BTN_PLUS_PRESSED || BTN_RIGHT_PRESSED)
		{
			_hideCheatSettings();
			if (m_cheatSettingsPage == (m_cheatfile.getCnt()+CHEATSPERPAGE-1)/CHEATSPERPAGE)
				m_cheatSettingsPage = 1;
			else if (m_cheatSettingsPage < (m_cheatfile.getCnt()+CHEATSPERPAGE-1)/CHEATSPERPAGE)
				++m_cheatSettingsPage;
			_showCheatSettings();

			m_btnMgr.click(m_wmote, m_cheatBtnPageP);
		}
		else if ((wii_btnsHeld & WBTN_2) && (wii_btnsHeld & WBTN_1)!=0)
		{
			remove(fmt("%s/%s.gct", m_cheatDir.c_str(), m_cf.getId().c_str()));
			remove(fmt("%s/%s.txt", m_txtCheatDir.c_str(), m_cf.getId().c_str()));
			m_gcfg2.remove(m_cf.getId(), "cheat");
			m_gcfg2.remove(m_cf.getId(), "hooktype");
			break;
		}
		else if (BTN_A_PRESSED)
		{
			m_btnMgr.click(m_wmote);
			if (m_btnMgr.selected(m_cheatBtnBack))
				break;
			else if (m_btnMgr.selected(m_cheatBtnPageM))
			{
				_hideCheatSettings();
				if (m_cheatSettingsPage == 1)
					m_cheatSettingsPage = (m_cheatfile.getCnt()+CHEATSPERPAGE-1)/CHEATSPERPAGE;
				else if (m_cheatSettingsPage > 1)
					--m_cheatSettingsPage;
				_showCheatSettings();
			}
			else if (m_btnMgr.selected(m_cheatBtnPageP))
			{
				_hideCheatSettings();
				if (m_cheatSettingsPage == (m_cheatfile.getCnt()+CHEATSPERPAGE-1)/CHEATSPERPAGE)
					m_cheatSettingsPage = 1;
				else if (m_cheatSettingsPage < (m_cheatfile.getCnt()+CHEATSPERPAGE-1)/CHEATSPERPAGE)
					++m_cheatSettingsPage;
				_showCheatSettings();
			}
				
			for (int i = 0; i < CHEATSPERPAGE; ++i)
				if (m_btnMgr.selected(m_cheatBtnItem[i]))
				{
					// handling code for clicked cheat
					m_cheatfile.sCheatSelected[(m_cheatSettingsPage-1)*CHEATSPERPAGE + i] = !m_cheatfile.sCheatSelected[(m_cheatSettingsPage-1)*CHEATSPERPAGE + i];
					_showCheatSettings();
				}
			
 			if (m_btnMgr.selected(m_cheatBtnApply))
			{
				bool selected = false;
				//checks if at least one cheat is selected
				for (unsigned int i=0; i < m_cheatfile.getCnt(); ++i)
				{
					if (m_cheatfile.sCheatSelected[i] == true) 
					{
						selected = true;
						break;
					}
				}
					
				if (selected)
				{
					m_cheatfile.createGCT(fmt("%s/%s.gct", m_cheatDir.c_str(), m_cf.getId().c_str())); 
					m_gcfg2.setOptBool(m_cf.getId(), "cheat", 1);
					m_gcfg2.setInt(m_cf.getId(), "hooktype", m_cfg.getInt(m_cf.getId(), "hooktype", 1));
				}
				else
				{
					remove(fmt("%s/%s.gct", m_cheatDir.c_str(), m_cf.getId().c_str()));
					m_gcfg2.remove(m_cf.getId(), "cheat");
					m_gcfg2.remove(m_cf.getId(), "hooktype");
				}
				m_cheatfile.createTXT(fmt("%s/%s.txt", m_txtCheatDir.c_str(), m_cf.getId().c_str()));
				break;
			}
			else if (m_btnMgr.selected(m_cheatBtnDownload))
			{
				int msg = 0;
				wstringEx prevMsg;
				
				// Download cheat code
				m_btnMgr.setProgress(m_downloadPBar, 0.f);
				_hideCheatSettings();
				_showCheatDownload();
				m_btnMgr.setText(m_downloadBtnCancel, _t("dl1", L"Cancel"));
				m_thrdStop = false;
				m_thrdMessageAdded = false;

				m_thrdWorking = true;
				lwp_t thread = LWP_THREAD_NULL;
				LWP_CreateThread(&thread, (void *(*)(void *))CMenu::_downloadCheatFileAsync, (void *)this, 0, 8192, 40);
				while (m_thrdWorking)
				{
					_mainLoopCommon(false, m_thrdWorking);
					if ((BTN_HOME_PRESSED || BTN_B_PRESSED) && !m_thrdWorking)
						break;
					if (BTN_A_PRESSED && !(m_thrdWorking && m_thrdStop))
					{
						m_btnMgr.click(m_wmote);
						if (m_btnMgr.selected(m_downloadBtnCancel))
						{
							LockMutex lock(m_mutex);
							m_thrdStop = true;
							m_thrdMessageAdded = true;
							m_thrdMessage = _t("dlmsg6", L"Canceling...");
						}
					}
					if (Sys_Exiting())
					{
						LockMutex lock(m_mutex);
						m_thrdStop = true;
						m_thrdMessageAdded = true;
						m_thrdMessage = _t("dlmsg6", L"Canceling...");
						m_thrdWorking = false;
					}

					if (m_thrdMessageAdded)
					{
						LockMutex lock(m_mutex);
						m_thrdMessageAdded = false;
						m_btnMgr.setProgress(m_downloadPBar, m_thrdProgress);
						if (m_thrdProgress >= 1.f) {
							// m_btnMgr.setText(m_downloadBtnCancel, _t("dl2", L"Back"));
							break;
						}
						if (prevMsg != m_thrdMessage)
						{
							prevMsg = m_thrdMessage;
							m_btnMgr.setText(m_downloadLblMessage[msg], m_thrdMessage, false);
							m_btnMgr.hide(m_downloadLblMessage[msg], 0, 0, -1.f, -1.f, true);
							m_btnMgr.show(m_downloadLblMessage[msg]);
							msg ^= 1;
							m_btnMgr.hide(m_downloadLblMessage[msg], 0, 0, -1.f, -1.f);
						}
					}
					if (m_thrdStop && !m_thrdWorking)
						break;
				}
				if (thread != LWP_THREAD_NULL)
				{
					LWP_JoinThread(thread, NULL);
					thread = LWP_THREAD_NULL;
				}
				_hideCheatDownload();
				
				m_cheatfile.openTxtfile(fmt("%s/%s.txt", m_txtCheatDir.c_str(), m_cf.getId().c_str()));
				_showCheatSettings();

				if (m_cheatfile.getCnt() == 0)
				{
					// cheat code not found, show result
					m_btnMgr.setText(m_cheatLblItem[0], _t("cheat4", L"Download not found."));
					m_btnMgr.setText(m_cheatLblItem[1], sfmt(GECKOURL, m_cf.getId().c_str()));
					m_btnMgr.show(m_cheatLblItem[1]);
				}
			}
		}
	}
	_hideCheatSettings();
}

void CMenu::_hideCheatSettings(bool instant)
{
	m_btnMgr.hide(m_cheatBtnBack, instant);
	m_btnMgr.hide(m_cheatBtnApply, instant);
	m_btnMgr.hide(m_cheatBtnDownload, instant);
	m_btnMgr.hide(m_cheatLblTitle, instant);

	m_btnMgr.hide(m_cheatLblPage, instant);
	m_btnMgr.hide(m_cheatBtnPageM, instant);
	m_btnMgr.hide(m_cheatBtnPageP, instant);
	
	for (int i=0;i<CHEATSPERPAGE;++i) {
		m_btnMgr.hide(m_cheatBtnItem[i], instant);
		m_btnMgr.hide(m_cheatLblItem[i], instant);
	}
	
	for (u32 i = 0; i < ARRAY_SIZE(m_cheatLblUser); ++i)
		if (m_cheatLblUser[i] != -1u)
			m_btnMgr.hide(m_cheatLblUser[i], instant);
}

// CheatMenu
// check for cheat txt file
// if it exists, load it and show cheat texts on screen
// if it does not exist, show download button
void CMenu::_showCheatSettings(void)
{
	_setBg(m_cheatBg, m_cheatBg);
	m_btnMgr.show(m_cheatBtnBack);
	m_btnMgr.show(m_cheatLblTitle);

	for (u32 i = 0; i < ARRAY_SIZE(m_cheatLblUser); ++i)
		if (m_cheatLblUser[i] != -1u)
			m_btnMgr.show(m_cheatLblUser[i]);

	if (m_cheatfile.getCnt() > 0) {

		// cheat found, show apply
		m_btnMgr.show(m_cheatBtnApply);
		m_btnMgr.show(m_cheatLblPage);
		m_btnMgr.show(m_cheatBtnPageM);
		m_btnMgr.show(m_cheatBtnPageP);
		m_btnMgr.setText(m_cheatLblPage, wfmt(L"%i / %i", m_cheatSettingsPage, (m_cheatfile.getCnt()+CHEATSPERPAGE-1)/CHEATSPERPAGE)); 
		
		// Show cheats if available, else hide
		for (u32 i=0; i < CHEATSPERPAGE; ++i) {
			// cheat in range?
			if (((m_cheatSettingsPage-1)*CHEATSPERPAGE + i + 1) <= m_cheatfile.getCnt()) 
			{
				//Limit to 70 characters otherwise the Cheatnames overlap
				char tempcheatname[71];
				strncpy(tempcheatname, m_cheatfile.getCheatName((m_cheatSettingsPage-1)*CHEATSPERPAGE + i).c_str(),70);
				tempcheatname[70] = '\0';
				
				// cheat avaiable, show elements and text
				m_btnMgr.setText(m_cheatLblItem[i], wstringEx(tempcheatname));
				m_btnMgr.setText(m_cheatBtnItem[i], _optBoolToString(m_cheatfile.sCheatSelected[(m_cheatSettingsPage-1)*CHEATSPERPAGE + i]));
				
				m_btnMgr.show(m_cheatLblItem[i], true);
				m_btnMgr.show(m_cheatBtnItem[i], true);
			}
			else
			{
				// cheat out of range, hide elements
				m_btnMgr.hide(m_cheatLblItem[i], true);
				m_btnMgr.hide(m_cheatBtnItem[i], true);
			}
		}


	}
	else
	{
		// no cheat found, allow downloading
		m_btnMgr.show(m_cheatBtnDownload);
		m_btnMgr.setText(m_cheatLblItem[0], _t("cheat3", L"Cheat file for game not found."));
		m_btnMgr.show(m_cheatLblItem[0]);
		
	}
}


void CMenu::_initCheatSettingsMenu(CMenu::SThemeData &theme)
{
	_addUserLabels(theme, m_cheatLblUser, ARRAY_SIZE(m_cheatLblUser), "CHEAT");
	m_cheatBg = _texture(theme.texSet, "CHEAT/BG", "texture", theme.bg);
	m_cheatLblTitle = _addLabel(theme, "CHEAT/TITLE", theme.lblFont, L"Cheats", 20, 30, 600, 60, theme.titleFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE);
	m_cheatBtnBack = _addButton(theme, "CHEAT/BACK_BTN", theme.btnFont, L"", 460, 410, 150, 56, theme.btnFontColor);
	m_cheatBtnApply = _addButton(theme, "CHEAT/APPLY_BTN", theme.btnFont, L"", 240, 410, 150, 56, theme.btnFontColor);
	m_cheatBtnDownload = _addButton(theme, "CHEAT/DOWNLOAD_BTN", theme.btnFont, L"", 240, 410, 200, 56, theme.btnFontColor);

	m_cheatLblPage = _addLabel(theme, "CHEAT/PAGE_BTN", theme.btnFont, L"", 76, 410, 80, 56, theme.btnFontColor, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE, theme.btnTexC);
	m_cheatBtnPageM = _addPicButton(theme, "CHEAT/PAGE_MINUS", theme.btnTexMinus, theme.btnTexMinusS, 20, 410, 56, 56);
	m_cheatBtnPageP = _addPicButton(theme, "CHEAT/PAGE_PLUS", theme.btnTexPlus, theme.btnTexPlusS, 156, 410, 56, 56);

	m_cheatLblItem[0] = _addLabel(theme, "CHEAT/ITEM_0", theme.lblFont, L"", 40, 100, 460, 56, theme.lblFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE);
	m_cheatBtnItem[0] = _addButton(theme, "CHEAT/ITEM_0_BTN", theme.btnFont, L"", 500, 100, 120, 56, theme.btnFontColor);
	m_cheatLblItem[1] = _addLabel(theme, "CHEAT/ITEM_1", theme.lblFont, L"", 40, 160, 460, 56, theme.lblFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE);
	m_cheatBtnItem[1] = _addButton(theme, "CHEAT/ITEM_1_BTN", theme.btnFont, L"", 500, 160, 120, 56, theme.btnFontColor);
	m_cheatLblItem[2] = _addLabel(theme, "CHEAT/ITEM_2", theme.lblFont, L"", 40, 220, 460, 56, theme.lblFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE);
	m_cheatBtnItem[2] = _addButton(theme, "CHEAT/ITEM_2_BTN", theme.btnFont, L"", 500, 220, 120, 56, theme.btnFontColor);
	m_cheatLblItem[3] = _addLabel(theme, "CHEAT/ITEM_3", theme.lblFont, L"", 40, 280, 460, 56, theme.lblFontColor, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE);
	m_cheatBtnItem[3] = _addButton(theme, "CHEAT/ITEM_3_BTN", theme.btnFont, L"", 500, 280, 120, 56, theme.btnFontColor);

	_setHideAnim(m_systemLblTitle, "CHEAT/TITLE", 0, 100, 0.f, 0.f);
	_setHideAnim(m_cheatBtnApply, "CHEAT/APPLY_BTN", 0, 0, -2.f, 0.f);
	_setHideAnim(m_cheatBtnBack, "CHEAT/BACK_BTN", 0, 0, -2.f, 0.f);
	_setHideAnim(m_cheatBtnDownload, "CHEAT/DOWNLOAD_BTN", 0, 0, -2.f, 0.f);
	_setHideAnim(m_cheatLblPage, "CHEAT/PAGE_BTN", 0, 200, 1.f, 0.f);
	_setHideAnim(m_cheatBtnPageM, "CHEAT/PAGE_MINUS", 0, 200, 1.f, 0.f);
	_setHideAnim(m_cheatBtnPageP, "CHEAT/PAGE_PLUS", 0, 200, 1.f, 0.f);
	
	for (int i=0;i<CHEATSPERPAGE;++i) {
		_setHideAnim(m_cheatLblItem[i], sfmt("CHEAT/ITEM_%i", i).c_str(), -200, 0, 1.f, 0.f);
		_setHideAnim(m_cheatBtnItem[i], sfmt("CHEAT/ITEM_%i_BTN", i).c_str(), 200, 0, 1.f, 0.f);
	}
	
	_hideCheatSettings();
	_textCheatSettings();
}

void CMenu::_textCheatSettings(void)
{
	m_btnMgr.setText(m_cheatBtnBack, _t("cheat1", L"Back"));
	m_btnMgr.setText(m_cheatBtnApply, _t("cheat2", L"Apply"));
	m_btnMgr.setText(m_cheatBtnDownload, _t("cfg4", L"Download"));
}
