#ifndef PAGEOPTIONS_H
#define PAGEOPTIONS_H

#include <QWidget>

namespace Ui {
class PageOptions;
}

class PageOptions : public QWidget
{
    Q_OBJECT

public:
    explicit PageOptions(QWidget *parent = nullptr);
    ~PageOptions();

private:
    Ui::PageOptions *ui;
};

#endif // PAGEOPTIONS_H
