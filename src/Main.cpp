// This file is part of BetterMultiSelection.
// 
// Copyright (C)2017 Justin Dailey <dail8859@yahoo.com>
// 
// BetterMultiSelection is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "AboutDialog.h"
#include "resource.h"
#include "PluginInterface.h"
#include "ScintillaEditor.h"

#include "UniConversion.h"
#include "GlobalMemory.h"

#include <algorithm>
#include <vector>
#include <sstream>

#define IsShiftPressed()   ((GetKeyState(VK_SHIFT) & KF_UP) != 0)
#define IsControlPressed() ((GetKeyState(VK_CONTROL) & KF_UP) != 0)
#define IsAltPressed()     ((GetKeyState(VK_MENU) & KF_UP) != 0)


static HANDLE _hModule;
static NppData nppData;
static HHOOK hook = NULL;
static bool hasFocus = true;
static ScintillaEditor editor;

static UINT cfMultiSelect = 0;
static UINT cfColumnSelect = 0;

static void enableBetterMultiSelection();
static void showAbout();

static LRESULT CALLBACK KeyboardProc(int ncode, WPARAM wparam, LPARAM lparam);

static FuncItem funcItem[] = {
	{ TEXT("Enable"), enableBetterMultiSelection, 0, false, nullptr },
	{ TEXT(""), nullptr, 0, false, nullptr },
	{ TEXT("About..."), showAbout, 0, false, nullptr }
};

static const wchar_t *GetIniFilePath() {
	static wchar_t iniPath[MAX_PATH] = { 0 };

	if (iniPath[0] == 0) {
		SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniPath);
		wcscat_s(iniPath, MAX_PATH, L"\\BetterMultiSelection.ini");
	}

	return iniPath;
}

static void enableBetterMultiSelection() {
	if (hook) {
		UnhookWindowsHookEx(hook);
		hook = NULL;
		SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[0]._cmdID, 0);
	}
	else {
		hook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, (HINSTANCE)_hModule, ::GetCurrentThreadId());
		SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[0]._cmdID, 1);
		editor.AutoCSetMulti(SC_MULTIAUTOC_EACH);
	}
}

static void showAbout() {
	ShowAboutDialog((HINSTANCE)_hModule, MAKEINTRESOURCE(IDD_ABOUTDLG), nppData._nppHandle);
}

static HWND GetCurrentScintilla() {
	int which = 0;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, SCI_UNUSED, (LPARAM)&which);
	return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

#include "../qt/SelectionCore.h"
// ============================================================================

// OpenClipboard may fail if another application has opened the clipboard.
// Try up to 8 times, with an initial delay of 1 ms and an exponential back off
// for a maximum total delay of 127 ms (1+2+4+8+16+32+64).
bool OpenClipboardRetry(HWND hwnd) {
	for (int attempt = 0; attempt < 8; attempt++) {
		if (attempt > 0) {
			Sleep(1 << (attempt - 1));
		}
		if (OpenClipboard(hwnd)) {
			return true;
		}
	}
	return false;
}

#include "../qt/CodePageCore.h"
// This is a modificated version of ScintillaWin::CopyToClipboard()
// Multilpe selects can be treated like rectangular and concat'ed together by newlines
bool CopyToClipboard(ScintillaEditor &editor) {
	if (!OpenClipboardRetry(editor.GetScintillaInstance())) {
		return false;
	}

	EmptyClipboard();

	GlobalMemory uniText;

	std::string selectedText;

	EditSelections([&selectedText, &editor](Selection& selection) {
		editor.SetTargetRange(selection.start(), selection.end());

		// TODO: check if newline in range and if so abort?
		// Newlines in the selection will mess up pasting since it
		// will look like an extra row

		selectedText.append(editor.GetTargetText());
		selectedText.append(StringFromEOLMode(editor.GetEOLMode()));
	});

	// Default Scintilla behaviour in Unicode mode
	if (editor.GetCodePage() == SC_CP_UTF8) {
		const size_t uchars = UTF16Length(selectedText.c_str(), selectedText.size());
		uniText.Allocate(2 * uchars);
		if (uniText) {
			UTF16FromUTF8(selectedText.c_str(), selectedText.size(), static_cast<wchar_t *>(uniText.ptr), uchars);
		}
	}
	else {
		// Not Unicode mode
		// Convert to Unicode using the current Scintilla code page
		const UINT cpSrc = CodePageFromCharSet(editor.StyleGetCharacterSet(STYLE_DEFAULT), editor.GetCodePage());
		const int uLen = MultiByteToWideChar(cpSrc, 0, selectedText.c_str(), static_cast<int>(selectedText.size()), 0, 0);
		uniText.Allocate(2 * uLen);
		if (uniText) {
			MultiByteToWideChar(cpSrc, 0, selectedText.c_str(), static_cast<int>(selectedText.size()), static_cast<wchar_t *>(uniText.ptr), uLen);
		}
	}

	if (uniText) {
		uniText.SetClip(CF_UNICODETEXT);
	}
	else {
		// There was a failure - try to copy at least ANSI text
		GlobalMemory ansiText;
		ansiText.Allocate(selectedText.size());
		if (ansiText) {
			memcpy(ansiText.ptr, selectedText.c_str(), selectedText.size());
			ansiText.SetClip(CF_TEXT);
		}
	}

	SetClipboardData(cfColumnSelect, 0);
	SetClipboardData(cfMultiSelect, 0);

	CloseClipboard();

	return true;
}

UINT CodePageOfDocument(ScintillaEditor &editor) {
	return CodePageFromCharSet(editor.StyleGetCharacterSet(STYLE_DEFAULT), editor.GetCodePage());
}

#include "../qt/PasteCore.h"
bool Paste(ScintillaEditor &editor) {
	if (!IsClipboardFormatAvailable(cfColumnSelect) && !IsClipboardFormatAvailable(cfMultiSelect))
		return false;

	if (!OpenClipboardRetry(editor.GetScintillaInstance())) {
		return false;
	}

	// Always use CF_UNICODETEXT if available
	GlobalMemory memUSelection(::GetClipboardData(CF_UNICODETEXT));
	if (memUSelection) {
		const wchar_t *uptr = static_cast<const wchar_t *>(memUSelection.ptr);
		if (uptr) {
			size_t len;
			std::vector<char> putf;
			// Default Scintilla behaviour in Unicode mode
			if (editor.GetCodePage() == SC_CP_UTF8) {
				const size_t bytes = memUSelection.Size();
				len = UTF8Length(uptr, bytes / 2);
				putf.resize(len + 1);
				UTF8FromUTF16(uptr, bytes / 2, &putf[0], len);
			}
			else {
				// CF_UNICODETEXT available, but not in Unicode mode
				// Convert from Unicode to current Scintilla code page
				const UINT cpDest = CodePageOfDocument(editor);
				len = WideCharToMultiByte(cpDest, 0, uptr, -1, NULL, 0, NULL, NULL) - 1; // subtract 0 terminator
				putf.resize(len + 1);
				WideCharToMultiByte(cpDest, 0, uptr, -1, &putf[0], static_cast<int>(len) + 1, NULL, NULL);
			}

			if (InsertMultiCursorPaste(editor, &putf[0])) {
				memUSelection.Unlock();
				CloseClipboard();
				return true;
			}
		}
		
	}
	else {
		// CF_UNICODETEXT not available, paste ANSI text
		GlobalMemory memSelection(::GetClipboardData(CF_TEXT));
		if (memSelection) {
			const char *ptr = static_cast<const char *>(memSelection.ptr);
			if (ptr) {
				const size_t bytes = memSelection.Size();
				size_t len = bytes;
				for (size_t i = 0; i < bytes; i++) {
					if ((len == bytes) && (0 == ptr[i]))
						len = i;
				}

				// In Unicode mode, convert clipboard text to UTF-8
				if (editor.GetCodePage() == SC_CP_UTF8) {
					std::vector<wchar_t> uptr(len + 1);

					const int ilen = static_cast<int>(len);
					const size_t ulen = ::MultiByteToWideChar(CP_ACP, 0, ptr, ilen, &uptr[0], ilen + 1);

					const size_t mlen = UTF8Length(&uptr[0], ulen);
					std::vector<char> putf(mlen + 1);
					UTF8FromUTF16(&uptr[0], ulen, &putf[0], mlen);

					if (InsertMultiCursorPaste(editor, &putf[0])) {
						memSelection.Unlock();
						CloseClipboard();
						return true;
					}
				}
				else {
					if (InsertMultiCursorPaste(editor, ptr)) {
						memSelection.Unlock();
						CloseClipboard();
						return true;
					}
				}
			}
		}
	}

	CloseClipboard();
	return false;
}

#include "../qt/KeyboardCore.h"
LRESULT CALLBACK KeyboardProc(int ncode, WPARAM wparam, LPARAM lparam) {
    if(ncode==HC_ACTION && (HIWORD(lparam)&KF_UP)==0 && !IsAltPressed() && hasFocus
        && HandleKey(static_cast<int>(wparam),IsControlPressed(),IsShiftPressed())) return TRUE;
    return CallNextHookEx(hook,ncode,wparam,lparam);
}
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved) {
	switch (reasonForCall) {
		case DLL_PROCESS_ATTACH:
			cfColumnSelect = RegisterClipboardFormat(L"MSDEVColumnSelect");
			cfMultiSelect = RegisterClipboardFormat(L"BMSMultiSelect");
			_hModule = hModule;
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	nppData = notepadPlusData;

	// Set this as early as possible so it is in a valid state
	editor.SetScintillaInstance(nppData._scintillaMainHandle);
}

extern "C" __declspec(dllexport) const wchar_t *getName() {
	return TEXT("BetterMultiSelection");
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) {
	*nbF = sizeof(funcItem) / sizeof(funcItem[0]);
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	switch (notifyCode->nmhdr.code) {
		case SCN_CHARADDED:
			//if (editor.GetSelections() > 1)

			break;
		case SCN_FOCUSIN:
			hasFocus = true;
			break;
		case SCN_FOCUSOUT:
			hasFocus = false;
			break;
		case NPPN_READY: {
			bool isEnabled = GetPrivateProfileInt(TEXT("BetterMultiSelection"), TEXT("enabled"), 1, GetIniFilePath()) == 1;
			if (isEnabled) {
				enableBetterMultiSelection();
			}
			break;
		}
		case NPPN_SHUTDOWN:
			WritePrivateProfileString(TEXT("BetterMultiSelection"), TEXT("enabled"), hook ? TEXT("1") : TEXT("0"), GetIniFilePath());
			if (hook != NULL)
				UnhookWindowsHookEx(hook);
			break;
		case NPPN_BUFFERACTIVATED:
			editor.SetScintillaInstance(GetCurrentScintilla());
			editor.AutoCSetMulti(SC_MULTIAUTOC_EACH);
			break;
	}
	return;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() {
	return TRUE;
}
#endif
