// Qt transport for the original BetterMultiSelection algorithms (GPL-2.0-or-later).
#pragma once
#include <Plugin.h>
class ScintillaEditor {
public:
    intptr_t Call(unsigned msg,uintptr_t w=0,intptr_t l=0) { return QtPlugin::sci(msg,w,l); }
    int GetSelections() { return Call(SCI_GETSELECTIONS); }
    int GetMainSelection() { return Call(SCI_GETMAINSELECTION); }
    int GetSelectionNCaret(int n) { return Call(SCI_GETSELECTIONNCARET,n); }
    int GetSelectionNAnchor(int n) { return Call(SCI_GETSELECTIONNANCHOR,n); }
    int GetSelectionNStart(int n) { return Call(SCI_GETSELECTIONNSTART,n); }
    int GetSelectionNEnd(int n) { return Call(SCI_GETSELECTIONNEND,n); }
    void SetSelection(int caret,int anchor) { Call(SCI_SETSELECTION,caret,anchor); }
    void AddSelection(int caret,int anchor) { Call(SCI_ADDSELECTION,caret,anchor); }
    void ClearSelections() { Call(SCI_CLEARSELECTIONS); }
    void BeginUndoAction() { Call(SCI_BEGINUNDOACTION); }
    void EndUndoAction() { Call(SCI_ENDUNDOACTION); }
    int GetLength() { return Call(SCI_GETLENGTH); }
    int GetEOLMode() { return Call(SCI_GETEOLMODE); }
    int GetCodePage() { return Call(SCI_GETCODEPAGE); }
    int StyleGetCharacterSet(int n) { return Call(SCI_STYLEGETCHARACTERSET,n); }
    bool GetPasteConvertEndings() { return Call(SCI_GETPASTECONVERTENDINGS); }
    void SetTargetRange(int a,int b) { Call(SCI_SETTARGETRANGE,a,b); }
    std::string GetTargetText() { return QtPlugin::targetText().toStdString(); }
    void ReplaceTarget(const std::string& text) { Call(SCI_REPLACETARGET,text.size(),reinterpret_cast<intptr_t>(text.data())); }
    int GetTargetEnd() { return Call(SCI_GETTARGETEND); }
    bool AutoCActive() { return Call(SCI_AUTOCACTIVE); }
    void AutoCSetMulti(int value) { Call(SCI_AUTOCSETMULTI,value); }
};
