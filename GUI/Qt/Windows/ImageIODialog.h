#ifndef IMAGEIODIALOG_H
#define IMAGEIODIALOG_H

#include <QDialog>

namespace Ui {
    class ImageIODialog;
}

class ImageIODialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageIODialog(QWidget *parent = 0);
    ~ImageIODialog();

private:
    Ui::ImageIODialog *ui;
};

#endif // IMAGEIODIALOG_H
