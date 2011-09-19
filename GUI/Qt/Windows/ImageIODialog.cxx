#include "ImageIODialog.h"
#include "ui_ImageIODialog.h"

ImageIODialog::ImageIODialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageIODialog)
{
    ui->setupUi(this);
}

ImageIODialog::~ImageIODialog()
{
    delete ui;
}
