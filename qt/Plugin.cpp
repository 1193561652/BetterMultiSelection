// Qt keyboard/clipboard adapter; original algorithms are shared with src/Main.cpp.
// Copyright (C)2017 Justin Dailey. GPL-2.0-or-later.
#include <Plugin.h>
#include <Clipboard.h>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QSettings>
#include <QTextCodec>
#include <QThread>
#include <algorithm>
#include <vector>
#include <sstream>
#include <cstring>
#include <memory>
#include "ScintillaEditor.h"
using namespace QtPlugin;
namespace {
ScintillaEditor editor;
#include "SelectionCore.h"
#include "PasteCore.h"
#include "CodePageCore.h"
QTextCodec* documentCodec() {
    auto cp=CodePageFromCharSet(editor.StyleGetCharacterSet(STYLE_DEFAULT),editor.GetCodePage());
    auto name=cp==SC_CP_UTF8?QByteArray("UTF-8"):cp==10000?QByteArray("macintosh"):
        cp==28605?QByteArray("ISO-8859-15"):QByteArray("CP")+QByteArray::number(cp);
    auto codec=cp==0 ? QTextCodec::codecForLocale() : QTextCodec::codecForName(name);
    if(!codec) throw std::runtime_error("Unsupported document character set");
    return codec;
}
bool CopyToClipboard(ScintillaEditor& editor) {
    std::string selectedText;
    EditSelections([&](Selection& selection) {
        editor.SetTargetRange(selection.start(),selection.end());
        selectedText.append(editor.GetTargetText());
        selectedText.append(StringFromEOLMode(editor.GetEOLMode()));
    });
    auto mime=std::make_unique<QMimeData>();
    mime->setText(documentCodec()->toUnicode(selectedText.data(),int(selectedText.size())));
    markRectangular(mime.get()); mime->setData("application/x-bms-multiselect",{});
#ifdef Q_OS_WIN
    mime->setData("BMSMultiSelect",{});
    mime->setData("application/x-qt-windows-mime;value=\"BMSMultiSelect\"",{});
#endif
    // Preserve the original retry/backoff and never cut after a failed clipboard write.
    auto clipboard=QApplication::clipboard();
    for(int attempt=0;attempt<8;++attempt) {
        if(attempt) QThread::msleep(1u<<(attempt-1));
        auto copy=new QMimeData;
        for(const auto& format:mime->formats()) copy->setData(format,mime->data(format));
        clipboard->setMimeData(copy);
        if(clipboard->ownsClipboard()) return true;
    }
    return false;
}
bool Paste(ScintillaEditor& editor) {
    auto mime=QApplication::clipboard()->mimeData();
    if(!mime || (!isRectangular(mime) && !mime->hasFormat("application/x-bms-multiselect") &&
        !mime->hasFormat("BMSMultiSelect") && !mime->hasFormat("application/x-qt-windows-mime;value=\"BMSMultiSelect\""))) return false;
    auto bytes=documentCodec()->fromUnicode(mime->text());
    return InsertMultiCursorPaste(editor,bytes.constData());
}
#include "KeyboardCore.h"
class KeyboardHook:public QObject {
public:
    bool enabled=false;
    bool eventFilter(QObject* receiver,QEvent* event) override {
        if(!host || !enabled || (event->type()!=QEvent::KeyPress && event->type()!=QEvent::ShortcutOverride) ||
            !(receiver->inherits("ScintillaEdit") || receiver->inherits("ScintillaEditBase"))) return false;
        auto keyEvent=static_cast<QKeyEvent*>(event);
        if(keyEvent->modifiers().testFlag(Qt::AltModifier)) return false;
        int key=keyEvent->key();
        switch(key) {
        case Qt::Key_Left:key=BmsKey::Left;break; case Qt::Key_Right:key=BmsKey::Right;break;
        case Qt::Key_Up:key=BmsKey::Up;break; case Qt::Key_Down:key=BmsKey::Down;break;
        case Qt::Key_Home:key=BmsKey::Home;break; case Qt::Key_End:key=BmsKey::End;break;
        case Qt::Key_Backspace:key=BmsKey::Back;break; case Qt::Key_Delete:key=BmsKey::Delete;break;
        case Qt::Key_Return:case Qt::Key_Enter:key=BmsKey::Return;break; case Qt::Key_Escape:key=BmsKey::Escape;break;
        }
        try {
            if(sci(SCI_GETLENGTH)>std::numeric_limits<int>::max()) return false;
            bool control=keyEvent->modifiers().testFlag(Qt::ControlModifier),shift=keyEvent->modifiers().testFlag(Qt::ShiftModifier);
            if(event->type()==QEvent::ShortcutOverride) {
                if(editor.GetSelections()<=1) return false;
                bool handled=control ? (key==BmsKey::Left || key==BmsKey::Right || (!shift &&
                    (key==BmsKey::Back || key==BmsKey::Delete || key=='X' || key=='C' || key=='V'))) :
                    (key==BmsKey::Escape || key==BmsKey::Left || key==BmsKey::Right || key==BmsKey::Home || key==BmsKey::End ||
                    key==BmsKey::Back || key==BmsKey::Delete || (!editor.AutoCActive() && (key==BmsKey::Return || key==BmsKey::Up || key==BmsKey::Down)));
                if(handled) { event->accept(); return true; }
                return false;
            }
            return HandleKey(key,control,shift);
        } catch(...) { return false; }
    }
};
std::unique_ptr<KeyboardHook> hook;
QString iniPath;
void enableBetterMultiSelection(void*) {
    hook->enabled=!hook->enabled; commands[0].initially_checked=hook->enabled;
    if(hook->enabled) editor.AutoCSetMulti(SC_MULTIAUTOC_EACH);
}
void showAbout(void*) { about("BetterMultiSelection","Copyright (C)2017 Justin Dailey <dail8859@yahoo.com>\nGPL-2.0-or-later\nhttps://github.com/dail8859/BetterMultiSelection"); }
void setup() {
    iniPath=configPath("BetterMultiSelection.ini"); hook=std::make_unique<KeyboardHook>();
    addToggle("Enable",invoke<enableBetterMultiSelection>,false); add("",nullptr); add("About...",invoke<showAbout>);
}
void notify(const NppPluginNotification* n) {
    switch(n->code) {
    case NPP_PLUGIN_NOTIFICATION_READY: {
        QSettings settings(iniPath,QSettings::IniFormat);
        hook->enabled=settings.value("BetterMultiSelection/enabled",1).toInt()==1;
        commands[0].initially_checked=hook->enabled;
        if(hook->enabled) editor.AutoCSetMulti(SC_MULTIAUTOC_EACH);
        qApp->installEventFilter(hook.get()); break;
    }
    case NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED: editor.AutoCSetMulti(SC_MULTIAUTOC_EACH); break;
    case NPP_PLUGIN_NOTIFICATION_SHUTDOWN: {
        QSettings settings(iniPath,QSettings::IniFormat); settings.setValue("BetterMultiSelection/enabled",hook->enabled?1:0);
        qApp->removeEventFilter(hook.get()); hook.reset(); break;
    }
    }
}
}
NPP_QT_EXPORTS("BetterMultiSelection",setup,notify)
