#pragma once
#include <QColor>
#include <QFont>
#include <QString>
#include <QFontDatabase>

struct Theme {
    // Background
    QColor bg          {0x12, 0x12, 0x14};
    QColor bgAlt       {0x1a, 0x1a, 0x1e};
    QColor bgCode      {0x1e, 0x1e, 0x24};
    QColor bgInlineCode{0x25, 0x25, 0x30};
    QColor bgQuote     {0x1a, 0x1a, 0x2a};
    QColor bgTable     {0x18, 0x18, 0x1e};
    QColor bgTableHead {0x20, 0x20, 0x2c};

    // Borders
    QColor border      {0x2a, 0x2a, 0x36};
    QColor borderAccent{0x5c, 0x6b, 0xc0};

    // Text
    QColor text        {0xe8, 0xe8, 0xf0};
    QColor textMuted   {0x88, 0x88, 0xa0};
    QColor textCode    {0xa8, 0xe6, 0xcf};
    QColor textLink    {0x74, 0xa9, 0xfa};
    QColor textH1      {0xff, 0xff, 0xff};
    QColor textH2      {0xe0, 0xe8, 0xff};
    QColor textH3      {0xc8, 0xd8, 0xff};
    QColor textQuote   {0x9a, 0xa0, 0xc8};
    QColor quoteBar    {0x5c, 0x6b, 0xc0};

    // Syntax highlight
    QColor synKeyword  {0xbb, 0x86, 0xfc};
    QColor synString   {0xa8, 0xe6, 0xcf};
    QColor synComment  {0x62, 0x64, 0x80};
    QColor synNumber   {0xff, 0xd0, 0x82};
    QColor synType     {0x80, 0xcb, 0xff};

    // TOC
    QColor tocBg       {0x0e, 0x0e, 0x12};
    QColor tocText     {0xb0, 0xb0, 0xc8};
    QColor tocHover    {0x22, 0x22, 0x32};
    QColor tocActive   {0x2c, 0x2c, 0x48};
    QColor tocAccent   {0x5c, 0x6b, 0xc0};

    // Header
    QColor headerBg    {0x0e, 0x0e, 0x12};
    QColor headerText  {0xd0, 0xd0, 0xe8};
};

class ThemeManager {
public:
    static ThemeManager& instance() {
        static ThemeManager t;
        return t;
    }

    const Theme& theme() const { return m_theme; }

    QFont fontBody(int ptSize = 11) const {
        QFont f = m_fontBody; f.setPointSize(ptSize); return f; }
    QFont fontBold(int ptSize = 11) const {
        QFont f = m_fontBold; f.setPointSize(ptSize); return f; }
    QFont fontItalic(int ptSize = 11) const {
        QFont f = m_fontItalic; f.setPointSize(ptSize); return f; }
    QFont fontMono(int ptSize = 10) const {
        QFont f = m_fontMono; f.setPointSize(ptSize); return f; }
    QFont fontHeading(int ptSize = 18) const {
        QFont f = m_fontHeading; f.setPointSize(ptSize); return f; }

private:
    ThemeManager() { loadFonts(); }

    void loadFonts() {
        // Body — system sans-serif stack
        m_fontBody = QFont("Segoe UI");
        m_fontBody.setStyleHint(QFont::SansSerif);

        m_fontBold = m_fontBody;
        m_fontBold.setBold(true);

        m_fontItalic = m_fontBody;
        m_fontItalic.setItalic(true);

        // Monospace
        m_fontMono = QFont("Consolas");
        m_fontMono.setStyleHint(QFont::Monospace);
        m_fontMono.setFixedPitch(true);

        // Heading
        m_fontHeading = QFont("Segoe UI Semibold");
        m_fontHeading.setStyleHint(QFont::SansSerif);
        m_fontHeading.setWeight(QFont::DemiBold);
    }

    Theme m_theme;
    QFont m_fontBody;
    QFont m_fontBold;
    QFont m_fontItalic;
    QFont m_fontMono;
    QFont m_fontHeading;
};
