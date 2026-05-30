#pragma once
#include <QMainWindow>
#include <QString>
#include <QTimer>

class MarkdownRenderer;
class TocWidget;
class MarkdownParser;
class QLabel;
class QSplitter;
class QPushButton;
class QFileSystemWatcher;
class QPropertyAnimation;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openFile(const QString &path);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void onOpenFile();
    void onReload();
    void onFileChanged(const QString &path);

private:
    void buildUi();
    void applyTheme();
    void loadMarkdown(const QString &path);
    void updateTitle(const QString &filename);

    // widgets
    QWidget           *m_header;
    QSplitter         *m_splitter;
    TocWidget         *m_toc;
    MarkdownRenderer  *m_renderer;
    QLabel            *m_titleLabel;
    QLabel            *m_statusLabel;
    QPushButton       *m_hamburger;
    QPushButton       *m_openBtn;
    QPushButton       *m_reloadBtn;

    // state
    MarkdownParser    *m_parser;
    QString            m_currentPath;
    QFileSystemWatcher *m_watcher;
    QTimer             *m_reloadTimer; // debounce
};
