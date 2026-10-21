#include "theme.hpp"
#include <QApplication>
#include <QPalette>
#include <QStyleHints>

void applyNightTheme(QApplication& app) {
    app.setStyle("Fusion");
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#171b22"));
    palette.setColor(QPalette::WindowText, QColor("#e1e7ef"));
    palette.setColor(QPalette::Base, QColor("#10151d"));
    palette.setColor(QPalette::AlternateBase, QColor("#1c2430"));
    palette.setColor(QPalette::Text, QColor("#e1e7ef"));
    palette.setColor(QPalette::Button, QColor("#242e3b"));
    palette.setColor(QPalette::ButtonText, QColor("#e1e7ef"));
    palette.setColor(QPalette::Highlight, QColor("#315a88"));
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::ToolTipBase, QColor("#242e3b"));
    palette.setColor(QPalette::ToolTipText, QColor("#e1e7ef"));
    palette.setColor(QPalette::PlaceholderText, QColor("#8996aa"));
    palette.setColor(QPalette::Link, QColor("#8ab4f8"));
    palette.setColor(QPalette::BrightText, QColor("#ff8d9c"));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        palette.setColor(QPalette::Disabled, role, QColor("#788394"));
    app.setPalette(palette);
    app.setStyleSheet(R"(
        QToolBar { border: 0; spacing: 6px; padding: 10px; background: #1c2430; }
        QToolButton { border: 1px solid transparent; border-radius: 5px; padding: 7px 10px; }
        QToolButton:hover { background: #2a3748; border-color: #3f526b; }
        QToolButton:pressed { background: #315a88; }
        QToolBar::separator { background: #364153; width: 1px; margin: 5px; }
        QPlainTextEdit, QLineEdit { border: 1px solid #303c4d; border-radius: 5px; padding: 8px; selection-background-color: #315a88; }
        QPlainTextEdit:focus, QLineEdit:focus { border-color: #668ec4; }
        QLineEdit:disabled { background: #1c2430; }
        QStatusBar { color: #a7b4c7; padding: 3px; }
        QSplitter::handle { background: #171b22; height: 9px; }
        QScrollBar:vertical { background: #171b22; width: 12px; margin: 0; }
        QScrollBar:horizontal { background: #171b22; height: 12px; margin: 0; }
        QScrollBar::handle { background: #3a485c; border-radius: 4px; min-width: 24px; min-height: 24px; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
        QToolTip { border: 1px solid #45566d; padding: 6px; }
    )");
}
