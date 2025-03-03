#ifndef FIELDTOOLBAR_H
#define FIELDTOOLBAR_H

#include <QWidget>

namespace Ui {
class FieldToolbar;
}

class FieldToolbar : public QWidget
{
    Q_OBJECT

public:
    explicit FieldToolbar(QWidget *parent = nullptr);
    ~FieldToolbar();

private:
    Ui::FieldToolbar *ui;
};

#endif // FIELDTOOLBAR_H
