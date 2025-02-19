#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QEvent>
#include <QResizeEvent>



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void resizeEvent(QResizeEvent* e) override
    {
        QMainWindow::resizeEvent(e);

        // Delay checking the final window size
        //QTimer::singleShot(0, this, [this]() {
        if (e->type() == QEvent::WindowStateChange) 
        {
            if (this->isMaximized())
                qDebug() << "MainWindow Size: " << size();
        }
        //});
    }
};
#endif // MAINWINDOW_H
