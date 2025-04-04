#include "mainwindow.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QMenuBar>
#include <QStyle>
#include "pagepreview.h"
#include "pageoptions.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    
    /*{
        QMenuBar* menuBar = new QMenuBar(this);
        setMenuBar(menuBar);

        //menuBar->setStyleSheet("\
        //    QMenuBar {                            \
        //        color: white;                     \
        //        background: #1e1e1e;              \
        //    }                                     \
        //    QMenu {                               \
        //        background-color: #1e1e1e;        \
        //        border: 1px solid black;          \
        //    }                                     \
        //    QMenu::item{                          \
        //        background-color: transparent;    \
        //    }                                     \
        //    QMenu::item:selected{                 \
        //        background-color: #4e4e4e;        \
        //    }"
        //);

        menuBar->setStyleSheet("\
            QMenuBar {                            \
                color: white;                     \
                background: #252525;              \
            }                                     \
        ");

        // Create File menu
        QMenu* fileMenu = menuBar->addMenu("File");

        // Add actions to the File menu
        //QAction* newAction = new QAction("New", this);
        //QAction* openAction = new QAction("Open", this);
        QAction* exitAction = new QAction("Exit", this);

        //fileMenu->addAction(newAction);
        //fileMenu->addAction(openAction);
        fileMenu->addSeparator(); // Optional: Add a separator
        fileMenu->addAction(exitAction);

        // Connect actions to slots
        //connect(newAction, &QAction::triggered, this, &QtSim::onSimSelector);
        connect(exitAction, &QAction::triggered, this, &MainWindow::close);

        // Create Help menu
        QMenu* helpMenu = menuBar->addMenu("Help");

        // Add About action to Help menu
        QAction* aboutAction = new QAction("About", this);
        helpMenu->addAction(aboutAction);

        //connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    }

    // Status bar
    QStatusBar* statusBar = new QStatusBar(this);
    statusBar->setStyleSheet("QStatusBar { background-color: #252525; }");

    statusBar->showMessage("No project active");
    setStatusBar(statusBar);*/

    /*const QString splitterSheet = "\
        QSplitter { background: #1e1e1e; } \
        QSplitter::handle {                       \
            image: url(:/res/splitter_handle.png); \
        }                                          \
        QSplitter::handle:horizontal{              \
            width: 6px;                            \
        }                                          \
        QSplitter::handle:vertical{              \
            height: 6px;                           \
        }";*/
    //QVBoxLayout *layout = new QVBoxLayout(this);
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    //mainSplitter->setStyleSheet(splitterSheet);
    //mainSplitter->setStyleSheet(splitterSheet
        //"QSplitter {\
        //    background-color: #fe1e1e;\
        //}\
        //QSplitter::handle {\
        //    color: rgb(0,0,0);\
        //    border: 3px dashed black;\
        //    margin: 1px 50px;\
        //    min - height: 10px;\
        //    max - height: 10px;\
        //}"
    //);

    //pageOptions = new PageOptions(statusBar, this);
    pageOptions = new PageOptions(nullptr, this);
    pagePreview = new PagePreview(this);
    pageOptions->setPagePreview(pagePreview);

    pageOptions->setAutoFillBackground(true);
    //qDebug() << "PageOptions objectName:" << pageOptions->objectName();

    //pageOptions->style()->polish()

    mainSplitter->addWidget(pageOptions);
    mainSplitter->addWidget(pagePreview);

    setCentralWidget(mainSplitter);
    //qDebug() << "Widget Effective Style:" << pageOptions->style()->objectName();

    //QTimer timer;
    //timer.singleShot(1000, [this]() {
    //    Instructions* instructions = new Instructions(this);
    //    instructions->setFixedSize(instructions->size());
    //    instructions->show();
    //});
}

MainWindow::~MainWindow() {}
