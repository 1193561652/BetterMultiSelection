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
struct Selection {
	int caret;
	int anchor;

	Selection(int caret, int anchor) : caret(caret), anchor(anchor) {}

	int start() const { return (std::min)(caret, anchor); }
	int end() const { return (std::max)(caret, anchor); }
	int length() const { return end() - start(); }
	void set(int pos) { anchor = caret = pos; }
	void offset(int offset) { anchor += offset; caret += offset; }
};

static std::vector<Selection> GetSelections() {
	std::vector<Selection> selections;

	int num = editor.GetSelections();
	for (int i = 0; i < num; ++i) {
		int caret = editor.GetSelectionNCaret(i);
		int anchor = editor.GetSelectionNAnchor(i);
		selections.emplace_back(Selection{ caret, anchor });
	}

	return selections;
}

static void SetSelections(const std::vector<Selection> &selections) {
	for (size_t i = 0; i < selections.size(); ++i) {
		if (i == 0)
			editor.SetSelection(selections[i].caret, selections[i].anchor);
		else
			editor.AddSelection(selections[i].caret, selections[i].anchor);
	}
}

template<typename It>
It uniquify(It begin, It const end)
{
	std::vector<It> v;
	v.reserve(static_cast<size_t>(std::distance(begin, end)));

	for (It i = begin; i != end; ++i)
		v.push_back(i);

	std::sort(v.begin(), v.end(), [](const auto &lhs, const auto &rhs) {
		return (*lhs).start() < (*rhs).start() || (!((*rhs).start() < (*lhs).start()) && (*lhs).end() < (*rhs).end());
	});

	v.erase(std::unique(v.begin(), v.end(), [](const auto &lhs, const auto &rhs) {
		return (*lhs).start() == (*rhs).start() && (*lhs).end() == (*rhs).end();
	}), v.end());

	std::sort(v.begin(), v.end());

	size_t j = 0;
	for (It i = begin; i != end && j != v.size(); ++i) {
		if (i == v[j]) {
			using std::iter_swap; iter_swap(i, begin);
			++j;
			++begin;
		}
	}
	return begin;
}

// Create a closure that simply calls a SCI_XXX message
static auto SimpleEdit(int message) {
	return [message](Selection &selection) {
		editor.SetSelection(selection.caret, selection.anchor);
		editor.Call(message);

		selection.caret = editor.GetSelectionNCaret(0);
		selection.anchor = editor.GetSelectionNAnchor(0);
	};
}

template<typename T>
static void EditSelections(T edit) {
	auto selections = GetSelections();

	editor.ClearSelections();

	std::sort(selections.begin(), selections.end(), [](const auto &lhs, const auto &rhs) {
		return lhs.start() < rhs.start() || (!(rhs.start() < lhs.start()) && lhs.end() < rhs.end());
	});

	editor.BeginUndoAction();

	int totalOffset = 0;
	for (auto &selection : selections) {
		selection.offset(totalOffset);
		const int length = editor.GetLength();

		edit(selection);

		totalOffset += editor.GetLength() - length;
	}

	editor.EndUndoAction();

	selections.erase(uniquify(selections.begin(), selections.end()), selections.end());

	SetSelections(selections);
}

std::string TransformLineEnds(const char *s, int eolModeWanted) {
	std::string dest;
	const size_t len = strlen(s);
	for (size_t i = 0; s[i]; i++) {
		if (s[i] == '\n' || s[i] == '\r') {
			if (eolModeWanted == SC_EOL_CR) {
				dest.push_back('\r');
			}
			else if (eolModeWanted == SC_EOL_LF) {
				dest.push_back('\n');
			}
			else { // eolModeWanted == SC_EOL_CRLF
				dest.push_back('\r');
				dest.push_back('\n');
			}
			if ((s[i] == '\r') && (i + 1 < len) && (s[i + 1] == '\n')) {
				i++;
			}
		}
		else {
			dest.push_back(s[i]);
		}
	}
	return dest;
}

const char *StringFromEOLMode(int eolMode) {
	if (eolMode == SC_EOL_CRLF) {
		return "\r\n";
	}
	else if (eolMode == SC_EOL_CR) {
		return "\r";
	}
	else {
		return "\n";
	}
}

template <typename T, typename U>
static std::string join(const std::vector<T> &v, const U &delim) {
	std::stringstream ss;
	for (size_t i = 0; i < v.size(); ++i) {
		if (i != 0) ss << delim;
		ss << v[i];
	}
	return ss.str();
}

template <typename T>
static std::vector<std::basic_string<T>> split(std::basic_string<T> const &str, const std::basic_string<T> &delim) {
	size_t start;
	size_t end = 0;
	std::vector<std::basic_string<T>> out;

	while ((start = str.find_first_not_of(delim, end)) != std::basic_string<T>::npos) {
		end = str.find(delim, start);
		out.push_back(str.substr(start, end - start));
	}

	return out;
}

bool AllSelectionsHaveText(ScintillaEditor &editor) {
	const int selections = editor.GetSelections();
	bool has_selections = true;

	for (int i = 0; i < editor.GetSelections(); ++i) {
		int start = editor.GetSelectionNStart(i);
		int end = editor.GetSelectionNEnd(i);

		if (start == end) {
			return false;
		}
	}

	return true;
}
