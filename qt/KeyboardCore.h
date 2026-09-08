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

#pragma once
namespace BmsKey {
enum { Back=8, Return=13, Escape=27, End=35, Home=36, Left=37, Up=38, Right=39, Down=40, Delete=46 };
}
static bool HandleKey(int key,bool control,bool shift) {
if (editor.GetSelections() > 1) {
			if (control) {
				if (key == BmsKey::Left) {
					EditSelections(SimpleEdit(shift ? SCI_WORDLEFTEXTEND : SCI_WORDLEFT));
					return true; // This key has been "handled" and won't propogate
				}
				else if (key == BmsKey::Right) {
					EditSelections(SimpleEdit(shift ? SCI_WORDRIGHTENDEXTEND : SCI_WORDRIGHT));
					return true;
				}
				else if (!shift) { // Handle CTRL+{} only, allow CTRL+SHIFT+{} to be used elsewhere
					if (key == BmsKey::Back) {
						EditSelections(SimpleEdit(SCI_DELWORDLEFT));
						return true;
					}
					else if (key == BmsKey::Delete) {
						EditSelections(SimpleEdit(SCI_DELWORDRIGHT));
						return true;
					}
					else if (key == 'X' || key == 'C') {
						if (CopyToClipboard(editor)) {
							if (key == 'X') {
								EditSelections(SimpleEdit(SCI_DELETEBACK));
							}
							return true;
						}
					}
					else if (key == 'V') {
						if (Paste(editor)) {
							return true;
						}
					}
				}
			}
			else {
				if (key == BmsKey::Escape) {
					int caret = editor.GetSelectionNCaret(editor.GetMainSelection());
					editor.SetSelection(caret, caret);
					return true;
				}
				else if (key == BmsKey::Left) {
					EditSelections(SimpleEdit(shift ? SCI_CHARLEFTEXTEND : SCI_CHARLEFT));
					return true;
				}
				else if (key == BmsKey::Right) {
					EditSelections(SimpleEdit(shift ? SCI_CHARRIGHTEXTEND : SCI_CHARRIGHT));
					return true;
				}
				else if (key == BmsKey::Home) {
					EditSelections(SimpleEdit(shift ? SCI_VCHOMEWRAPEXTEND : SCI_VCHOMEWRAP));
					return true;
				}
				else if (key == BmsKey::End) {
					EditSelections(SimpleEdit(shift ? SCI_LINEENDWRAPEXTEND : SCI_LINEENDWRAP));
					return true;
				}
				else if (key == BmsKey::Back) {
					EditSelections(SimpleEdit(SCI_DELETEBACK));
					return true;
				}
				else if (key == BmsKey::Delete) {
					EditSelections(SimpleEdit(SCI_CLEAR));
					return true;
				}
				else if (key == BmsKey::Return) {
					if (!editor.AutoCActive()) {
						EditSelections(SimpleEdit(SCI_NEWLINE));
						return true;
					}
					// else just let Scintilla handle the insertion of autocompletion
				}
				else if (key == BmsKey::Up) {
					if (!editor.AutoCActive()) {
						EditSelections(SimpleEdit(shift ? SCI_LINEUPEXTEND : SCI_LINEUP));
						return true;
					}
					// else just let Scintilla handle the navigation of autocompletion
				}
				else if (key == BmsKey::Down) {
					if (!editor.AutoCActive()) {
						EditSelections(SimpleEdit(shift ? SCI_LINEDOWNEXTEND : SCI_LINEDOWN));
						return true;
					}
					// else just let Scintilla handle the navigation of autocompletion
				}
			}
		}
	return false;
}
