#include "message_dialog.hpp"
#include "theme.hpp"
#include <QApplication>
#include <QMessageBox>
#include <QStringView>
#include <iostream>

namespace abc {
namespace {
[[noreturn]] void unavailable(SourceLocation location) {
    throw Error("E_UI",location,"ShowMessage cannot open a graphical display",
        "ShowMessage не может открыть графический экран");
}

void ensure_application(SourceLocation location) {
    if(qobject_cast<QApplication*>(QCoreApplication::instance()))return;
    if(QCoreApplication::instance())unavailable(location);
#ifdef Q_OS_LINUX
    if(qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
        && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))unavailable(location);
#endif
    struct Application {
        int argc=1;
        char name[11]="ABC Native";
        char* argv[2]{name,nullptr};
        QApplication app{argc,argv};
        Application(){applyNightTheme(app);}
    };
    static Application application;
}
} // namespace

void show_qt_message(std::u16string_view text,std::u16string_view title,SourceLocation location) {
    ensure_application(location);
    std::cout.flush();
    QMessageBox dialog;
    dialog.setTextFormat(Qt::PlainText);
    dialog.setWindowTitle(QStringView(title).toString());
    dialog.setText(QStringView(text).toString());
    dialog.setStandardButtons(QMessageBox::Ok);
    dialog.exec();
}
} // namespace abc
