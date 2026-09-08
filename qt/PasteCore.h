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
bool InsertMultiCursorPaste(ScintillaEditor &editor, const char *text) {
	std::string st;

	if (editor.GetPasteConvertEndings()) {
		st = TransformLineEnds(text, editor.GetEOLMode());
	}
	else {
		st = text;
	}

	auto lines = split(st, std::string(StringFromEOLMode(editor.GetEOLMode())));
	if (lines.size() == editor.GetSelections()) {
		EditSelections([&lines, &editor](Selection &selection) {
			if (selection.caret < selection.anchor)
				editor.SetTargetRange(selection.caret, selection.anchor);
			else
				editor.SetTargetRange(selection.anchor, selection.caret);

			editor.ReplaceTarget(lines[0]);

			selection.caret = editor.GetTargetEnd();
			selection.anchor = editor.GetTargetEnd();

			// pop front
			lines.erase(lines.cbegin());
		});

		return true;
	}

	return false;
}
