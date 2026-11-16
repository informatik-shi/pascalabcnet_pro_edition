#include "../src/ide/message_dialog.hpp"
#include "../src/ide/theme.hpp"
#include <QApplication>
#include <QMessageBox>
#include <QPalette>
#include <QTimer>
#include <iostream>
#include <sstream>

int main(int argc,char** argv) {
    QApplication app(argc,argv);
    applyNightTheme(app);
    bool matched=false;
    QTimer::singleShot(0,&app,[&]{
        auto* dialog=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if(!dialog)return;
        matched=dialog->windowTitle()==QStringLiteral("Заголовок 😀")
            && dialog->text()==QStringLiteral("<b>Текст 😀</b>\nстрока")
            && dialog->textFormat()==Qt::PlainText
            && dialog->standardButtons()==QMessageBox::Ok
            && dialog->palette().color(QPalette::Window)==QColor("#171b22");
        dialog->accept();
    });
    const auto program=abc::compile(
        "uses Utils;begin ShowMessage('<b>Текст 😀</b>'#10'строка','Заголовок 😀');Writeln('after');end.");
    abc::ExecutionOptions options;
    options.show_message=abc::show_qt_message;
    std::istringstream input;
    std::ostringstream output;
    if(abc::execute(program,input,output,options)!=0 || !matched || output.str()!="after\n"){
        std::cerr<<"The modal Qt dialog did not preserve Unicode, plain text and the night theme\n";
        return 1;
    }
    return 0;
}
