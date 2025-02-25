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

    {
        QMenuBar* menuBar = new QMenuBar(this);
        setMenuBar(menuBar);

        menuBar->setStyleSheet("QMenuBar { color: rgb(255,255,255); background-color: #1e1e1e; }");

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
    statusBar->setStyleSheet("QStatusBar { color: rgb(255,255,255); background-color: #1e1e1e; }");

    statusBar->showMessage("No project active");
    setStatusBar(statusBar);

    //QVBoxLayout *layout = new QVBoxLayout(this);
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    //mainSplitter->setStyleSheet("QSplitter:handle { color: rgb(0,0,0); background: #1e1e1e; }");

    pageOptions = new PageOptions(statusBar, this);
    pagePreview = new PagePreview(this);
    pageOptions->setPagePreview(pagePreview);

    pageOptions->setAutoFillBackground(true);
    qDebug() << "PageOptions objectName:" << pageOptions->objectName();

    //pageOptions->style()->polish()

    mainSplitter->addWidget(pageOptions);
    mainSplitter->addWidget(pagePreview);

    setCentralWidget(mainSplitter);
    qDebug() << "Widget Effective Style:" << pageOptions->style()->objectName();
}

MainWindow::~MainWindow() {}
