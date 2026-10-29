#include "theme.hpp"
#include <QApplication>
#include <QAction>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFontDatabase>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSplitter>
#include <QStatusBar>
#include <QStringConverter>
#include <QSyntaxHighlighter>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <memory>

namespace {
const QString example = QStringLiteral(
    "begin\n  var число: integer;\n  Write('Введите число: ');\n  Readln(число);\n"
    "  Writeln('Результат: ', число * 2);\nend.\n");

class Highlighter final : public QSyntaxHighlighter {
public:
    explicit Highlighter(QTextDocument* document) : QSyntaxHighlighter(document) {}
protected:
    void highlightBlock(const QString& text) override {
        const QRegularExpression tokens(QStringLiteral(
            "\\b(begin|end|program|uses|type|var|const|out|params|array|of|record|function|procedure|new|default|nil|exit|result|list|stack|queue|dynarray|simpleset|assocarray|linkedlist|linkedlistnode|singlenode|integer|int64|real|double|boolean|string|char|word|byte|shortint|smallint|longword|uint64|biginteger|addr32|calcip|sequence|text|textfile|if|then|else|while|do|for|foreach|in|to|downto|repeat|until|break|continue|and|or|xor|not|shl|shr|div|mod|true|false)\\b"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
        auto matches = tokens.globalMatch(text);
        while (matches.hasNext()) {
            const auto match = matches.next();
            setFormat(match.capturedStart(), match.capturedLength(), QColor("#8ab4f8"));
        }
        const QRegularExpression functions(QStringLiteral(
            "\\b(swapsubstr|replacelast|replacemany|translate|ismatch|matchvalue|truetable|truetableprint|genaddr|genaddrbin|addrformat|padleft|padright|substring|remove|toarray|tolist|seq|seqfill|range|partitionpoints|linspace|sum|average|product|partialsum|select|selectmany|where|all|asenumerable|sequenceequal|defaultifempty|take|skip|takelast|skiplast|any|single|singleordefault|firstordefault|lastordefault|elementat|elementatordefault|random|randomize|setrandomseed|randomreal|arrrandom|arrrandominteger|arrrandomreal|createrandomintegerarray|createrandomrealarray|fillrandom|fillbyrandom|reverse|shuffle|swap|atan2|hypot|lerp|remap|gcd|lcm|нод|нок|uppercase|lowercase|upcase|lowcase|toupper|tolower|isletter|isdigit|isupper|islower|code|writearray|writelnarray|readarray|writematrix|rowcount|colcount|getlength|getupperbound|getlowerbound|rank|formatvalue|createrandomintegermatrix|createrandomrealmatrix|matrrandom|matrrandominteger|matrrandomreal|abs|sqr|sign|min|max|sin|sinh|cos|cosh|tan|tanh|arcsin|arccos|arctan|exp|ln|log|log2|log10|logn|sqrt|cbrt|isqrt|power|round|trunc|int|frac|floor|ceil|degtorad|radtodeg|length|copy|pos|posex|lastpos|concat|trim|trimleft|trimright|leftstr|rightstr|reversestring|comparestr|inttostr|strtoint|strtoint64|tobiginteger|strtoreal|strtofloat|floattostr|setlength|insert|delete|inc|dec|stringofchar|todigit|assert|ord|ordunicode|chr|chrunicode|succ|pred|write|writeln|read|readln|assign|assignfile|reset|rewrite|append|close|closefile|flush|eof|eoln|seekeof|seekeoln|openread|openwrite|openappend|readinteger|readint64|readbiginteger|readreal|readboolean|readstring|readlninteger|readlnint64|readlnbiginteger|readlnreal|readlnboolean|readlnstring|readlexem|readword|readchar|readlnchar|readtoend|name|fullname|arr|high|sort|sortdescending|fillbyvalue|readfiletoend|writestringtofile|readalltext|writealltext|readalllines|writealllines|fileexists|getdir|getcurrentdir|chdir|setcurrentdir|mkdir|createdir|rmdir|removedir|deletefile|erase|rename|renamefile|renamedirectory|changefilenameextension|count|add|removeat|clear|contains|push|pop|enqueue|dequeue|top|isempty|tostring|print|println|capacity|reserve|resize|find|remove|elem|min_cap|inc_cap_factor|first|last|prev|next|value|data|addfirst|addlast|addbefore|addafter|removefirst|removelast)\\b"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
        auto calls = functions.globalMatch(text);
        while (calls.hasNext()) {
            const auto match = calls.next();
            setFormat(match.capturedStart(), match.capturedLength(), QColor("#c6a8ff"));
        }
        // A single pass keeps comment delimiters inside strings inert.
        int state = previousBlockState() > 0 ? previousBlockState() : 0;
        for (qsizetype i = 0; i < text.size();) {
            const auto start = i;
            if (state == 0 && text.mid(i, 2) == "//") {
                setFormat(i, text.size() - i, QColor("#8e9aa9")); break;
            }
            if (state == 0 && text[i] == '\'') {
                ++i;
                while (i < text.size()) {
                    if (text[i++] == '\'') {
                        if (i < text.size() && text[i] == '\'') ++i;
                        else break;
                    }
                }
                setFormat(start, i - start, QColor("#a9d18e")); continue;
            }
            if (state == 0 && text[i] == '{') { state = 1; ++i; }
            else if (state == 0 && text.mid(i, 2) == "(*") { state = 2; i += 2; }
            else if (state == 0) { ++i; continue; }
            while (i < text.size()) {
                if (state == 1 && text[i] == '}') { ++i; state = 0; break; }
                if (state == 2 && text.mid(i, 2) == "*)") { i += 2; state = 0; break; }
                ++i;
            }
            setFormat(start, i - start, QColor("#8e9aa9"));
        }
        setCurrentBlockState(state);
    }
};

class Window final : public QMainWindow {
public:
    explicit Window(bool smoke, bool runtimeError, QString screenshot)
        : smoke_(smoke || runtimeError), runtime_error_(runtimeError), screenshot_(std::move(screenshot)) {
        setWindowTitle(QStringLiteral("ABC Native — Qt — прототип"));
        resize(1100, 760);
        auto* toolbar = addToolBar(QStringLiteral("Программа"));
        toolbar->setMovable(false);
        toolbar->addAction(QStringLiteral("Новый"), this, [this] {
            if (!confirmSave()) return;
            filename_.clear(); editor_->setPlainText(example); editor_->document()->setModified(false);
        });
        toolbar->addAction(QStringLiteral("Открыть"), this, [this] { open(); });
        toolbar->addAction(QStringLiteral("Сохранить"), this, [this] { save(); });
        toolbar->addSeparator();
        check_ = toolbar->addAction(QStringLiteral("Проверить"), this, [this] { start("check"); });
        run_ = toolbar->addAction(QStringLiteral("Запустить"), this, [this] { start("run"); });
        stop_ = toolbar->addAction(QStringLiteral("Остановить"), this, [this] { process_.kill(); });
        toolbar->addSeparator();
        toolbar->addAction(QStringLiteral("О проекте"), this, [this] {
            QMessageBox::about(this, QStringLiteral("ABC Native"), QStringLiteral(
                "Экспериментальный редактор Pascal на Qt.\nЯдро: C++20.\n\n"
                "Авторы PascalABC.NET:\nIvan Bondarev, Stanislav Mikhalkovich.\n"
                "Лицензии находятся в каталоге licenses поставки."));
        });
        stop_->setEnabled(false);
        run_->setShortcut(Qt::Key_F9);
        check_->setShortcut(Qt::Key_F8);

        auto* central = new QWidget;
        auto* layout = new QVBoxLayout(central);
        auto* splitter = new QSplitter(Qt::Vertical);
        editor_ = new QPlainTextEdit;
        auto codeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        codeFont.setPointSize(12);
        editor_->setFont(codeFont);
        editor_->setTabStopDistance(editor_->fontMetrics().horizontalAdvance(' ') * 2);
        editor_->setLineWrapMode(QPlainTextEdit::NoWrap);
        new Highlighter(editor_->document());
        editor_->setPlainText(runtime_error_ ? QStringLiteral("begin\n  var ноль := 0;\n  Writeln(10 div ноль);\nend.\n") : example);
        editor_->document()->setModified(false);
        output_ = new QPlainTextEdit;
        output_->setReadOnly(true);
        output_->setFont(editor_->font());
        output_->setMaximumBlockCount(10000);
        output_->setPlaceholderText(QStringLiteral("Здесь появятся результат проверки и вывод программы"));
        splitter->addWidget(editor_); splitter->addWidget(output_);
        splitter->setSizes({470, 180});
        input_ = new QLineEdit;
        input_->setPlaceholderText(QStringLiteral("Ввод для Readln — введите значение и нажмите Enter"));
        input_->setEnabled(false);
        layout->addWidget(splitter); layout->addWidget(input_);
        setCentralWidget(central);
        statusBar()->showMessage(QStringLiteral("Pascal • C++ ядро • F8 проверка • F9 запуск"));

        connect(input_, &QLineEdit::returnPressed, this, [this] {
            process_.write(input_->text().toUtf8() + '\n'); input_->clear();
        });
        connect(&process_, &QProcess::readyReadStandardOutput, this, [this] { append(stdout_decoder_(process_.readAllStandardOutput())); });
        connect(&process_, &QProcess::readyReadStandardError, this, [this] { readErrors(); });
        connect(&process_, &QProcess::started, this, [this] {
            if (smoke_) { if (!runtime_error_) process_.write("21\n"); process_.closeWriteChannel(); }
        });
        connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                append(QStringLiteral("Не удалось запустить ядро: ") + process_.errorString());
                running(false);
                if (smoke_) QApplication::exit(1);
            }
        });
        connect(&process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
            append(stdout_decoder_(process_.readAllStandardOutput()));
            readErrors();
            if (!stderr_pending_.isEmpty()) { append(displayError(stderr_pending_), true); stderr_pending_.clear(); }
            append(QStringLiteral("\n[Завершено, код %1]\n").arg(code));
            running(false);
            snapshot_.reset();
            if (smoke_) {
                const auto output = output_->toPlainText();
                const bool success = status == QProcess::NormalExit && (runtime_error_
                    ? code == 1 && output.contains("EN: Division by zero")
                        && output.contains(QStringLiteral("RU: Деление на ноль"))
                        && output.contains(QStringLiteral("Без имени.pas:3:"))
                    : code == 0 && output.contains(QStringLiteral("Результат: 42")));
                QTimer::singleShot(100, this, [this, success] {
                    bool saved = screenshot_.isEmpty() || grab().save(screenshot_);
                    QApplication::exit(success && saved ? 0 : 1);
                });
            }
        });
        if (smoke_) {
            QTimer::singleShot(0, this, [this] { start("run"); });
            QTimer::singleShot(15000, this, [] { QApplication::exit(1); });
        }
    }
    ~Window() override { process_.kill(); process_.waitForFinished(1000); }
protected:
    void closeEvent(QCloseEvent* event) override {
        if (!confirmSave()) event->ignore();
        else { process_.kill(); event->accept(); }
    }
private:
    void append(const QString& text, bool error = false) {
        auto cursor = output_->textCursor();
        cursor.movePosition(QTextCursor::End);
        QTextCharFormat format;
        format.setForeground(QColor(error ? "#ff8d9c" : "#e1e7ef"));
        cursor.insertText(text, format);
        output_->setTextCursor(cursor); output_->ensureCursorVisible();
    }
    QString displayError(QString text) const {
        if (snapshot_) text.replace(snapshot_->filePath("program.pas"),
            filename_.isEmpty() ? QStringLiteral("Без имени.pas") : filename_);
        return text;
    }
    void readErrors() {
        stderr_pending_ += stderr_decoder_(process_.readAllStandardError());
        qsizetype newline;
        while ((newline = stderr_pending_.indexOf('\n')) >= 0) {
            append(displayError(stderr_pending_.left(newline + 1)), true);
            stderr_pending_.remove(0, newline + 1);
        }
    }
    bool confirmSave() {
        if (!editor_->document()->isModified()) return true;
        const auto answer = QMessageBox::question(this, QStringLiteral("Изменения"), QStringLiteral("Сохранить изменения программы?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (answer == QMessageBox::Cancel) return false;
        return answer != QMessageBox::Save || save();
    }
    bool save() {
        auto target = filename_;
        if (target.isEmpty()) target = QFileDialog::getSaveFileName(this, QStringLiteral("Сохранить программу"), {}, "Pascal (*.pas)", nullptr, QFileDialog::DontUseNativeDialog);
        if (target.isEmpty()) return false;
        QSaveFile file(target);
        const auto data = editor_->toPlainText().toUtf8();
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
            QMessageBox::warning(this, QStringLiteral("Сохранение"), file.errorString()); return false;
        }
        filename_ = target; editor_->document()->setModified(false); return true;
    }
    void open() {
        if (!confirmSave()) return;
        const auto target = QFileDialog::getOpenFileName(this, QStringLiteral("Открыть программу"), {}, "Pascal (*.pas)", nullptr, QFileDialog::DontUseNativeDialog);
        if (target.isEmpty()) return;
        QFile file(target);
        if (!file.open(QIODevice::ReadOnly) || file.size() > 1'048'576) {
            QMessageBox::warning(this, QStringLiteral("Открытие"), QStringLiteral("Файл недоступен или превышает 1 МиБ")); return;
        }
        QStringDecoder decoder(QStringDecoder::Utf8);
        const QString text = decoder(file.readAll());
        if (decoder.hasError()) { QMessageBox::warning(this, QStringLiteral("Кодировка"), QStringLiteral("Для прототипа нужен файл UTF-8")); return; }
        filename_ = target; editor_->setPlainText(text); editor_->document()->setModified(false);
    }
    void running(bool value) {
        check_->setEnabled(!value); run_->setEnabled(!value); stop_->setEnabled(value); input_->setEnabled(value);
        statusBar()->showMessage(value ? QStringLiteral("Выполняется…") : QStringLiteral("Готово"));
    }
    void start(const QString& command) {
        if (process_.state() != QProcess::NotRunning) return;
        snapshot_ = std::make_unique<QTemporaryDir>();
        QFile source(snapshot_->filePath("program.pas"));
        const auto data = editor_->toPlainText().toUtf8();
        if (!snapshot_->isValid() || !source.open(QIODevice::WriteOnly) || source.write(data) != data.size()) {
            QMessageBox::warning(this, QStringLiteral("Запуск"), QStringLiteral("Не удалось сохранить снимок программы"));
            if (smoke_) QApplication::exit(1);
            return;
        }
        source.close();
        output_->clear(); stderr_pending_.clear(); stdout_decoder_.resetState(); stderr_decoder_.resetState();
        const auto runner = QCoreApplication::applicationDirPath() + "/abc"
#ifdef Q_OS_WIN
            ".exe"
#endif
            ;
        process_.setWorkingDirectory(filename_.isEmpty() ? QDir::homePath() : QFileInfo(filename_).absolutePath());
        running(true);
        process_.start(runner, {command, source.fileName()});
    }
    bool smoke_, runtime_error_;
    QString screenshot_, filename_, stderr_pending_;
    QPlainTextEdit *editor_ = nullptr, *output_ = nullptr;
    QLineEdit* input_ = nullptr;
    QAction *check_ = nullptr, *run_ = nullptr, *stop_ = nullptr;
    QProcess process_;
    QStringDecoder stdout_decoder_{QStringDecoder::Utf8}, stderr_decoder_{QStringDecoder::Utf8};
    std::unique_ptr<QTemporaryDir> snapshot_;
};
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("ABC Native Qt");
    applyNightTheme(app);
    QString screenshot;
    const auto arguments = app.arguments();
    const auto screenshot_index = arguments.indexOf("--screenshot");
    if (screenshot_index >= 0 && screenshot_index + 1 < arguments.size()) screenshot = arguments[screenshot_index + 1];
    Window window(arguments.contains("--smoke-test"), arguments.contains("--smoke-runtime-error"), screenshot);
    window.show();
    return app.exec();
}
