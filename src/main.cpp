#include "mainwindow.h"
#include <QScreen>
#include <QApplication>
#include <QFile>
#include <QStyleFactory>

#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#include "clipper2/clipper.h"

//#include <QFontDatabase>

/*void copyQtLogo()
{
    QString targetPath = QCoreApplication::applicationDirPath() + "/qtlogo.svg";
    if (!QFile::exists(targetPath)) {
        QFile::copy(":/res/logo.svg", targetPath);
    }
}*/


int main(int argc, char *argv[])
{
    
    //QApplication::setStyle("fusion");

    //qDebug() << "palette: " << QPalette();
    //qputenv("QT_QPA_PLATFORM", "windows:darkmode=2");
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

    /*int fontId = QFontDatabase::addApplicationFont(":/res/code128.ttf");
    if (fontId == -1) {
        qWarning() << "Failed to load barcode font!";
        return -1;
    }*/

    /*QPalette palette = a.palette();

    // Set the text colors you want (for instance, white)
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);

    // Apply the palette globally
    a.setPalette(palette);*/

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(50, 50, 50));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(40, 40, 40));
    darkPalette.setColor(QPalette::AlternateBase, QColor(60, 60, 60));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::green);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    a.setPalette(darkPalette);

    QFont defaultFont = a.font();
    defaultFont.setPointSize(9); // Set a fixed font size
    a.setFont(defaultFont);


    MainWindow window;

    

    //window.setStyleSheet("background: #666670;");
    window.resize(800, 600);
    window.setAutoFillBackground(true);

    #ifdef __EMSCRIPTEN__
    window.setWindowFlags(Qt::FramelessWindowHint);
    #else
    QList<QScreen*> screens = QGuiApplication::screens();
    int screenIndex = 4; // Change this to select the desired screen (0 = primary, 1 = second monitor, etc.)

    if (screenIndex >= 0 && screenIndex < screens.size()) {
        QScreen* targetScreen = screens.at(screenIndex);

        // Get the geometry of the target screen
        QRect screenGeometry = targetScreen->geometry();

        // Move the window to the center of the target screen
        int x = screenGeometry.x() + (screenGeometry.width() - window.width()) / 2;
        int y = screenGeometry.y() + (screenGeometry.height() - window.height()) / 2;
        window.move(x, y);

        qDebug() << "Window moved to screen:" << screenIndex;
    }
    else {
        qWarning() << "Invalid screen index!";
    }
    #endif

    qDebug() << "Maximized Window Size: " << window.size();
    window.setWindowTitle("Label Maker");
    window.setWindowIcon(QIcon(":/res/sonae_icon.png"));
    //window.setStyleSheet("QLabel { background: red; }");
    window.showMaximized();
    qDebug() << "Maximized Window Size: " << window.size();

    //window.dumpObjectTree();

    //QFile file(":/res/styles.qss");
    //if (file.open(QFile::ReadOnly)) {
    //    QString stylesheet = QLatin1String(file.readAll());
    //    
    //}

    return a.exec();
}
