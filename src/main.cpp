#include "MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    // High-DPI support
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("MarkdownViewer");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("mdviewer");

    // Prefer Fusion style for consistent cross-platform look
    if (QStyleFactory::keys().contains("Fusion"))
        app.setStyle(QStyleFactory::create("Fusion"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "Markdown file to open");
    parser.process(app);

    MainWindow win;
    win.show();

    const auto args = parser.positionalArguments();
    if (!args.isEmpty()) {
        win.openFile(args.first());
    }

    return app.exec();
}
