#include "QtWarningDialog.h"

#include "ui_QtWarningDialog.h"

#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <iostream>

void QtWarningDialog
::show(const std::vector<IRISWarning> &wl)
{
  const QString htmlTemplate =
      "<tr><td width=40><img src=\":/root/%1.png\" /></td>"
      "<td>%2</td></tr>";

  if(wl.size())
    {
    QString html;
    for(std::vector<IRISWarning>::const_iterator it = wl.begin();
        it != wl.end(); it++)
      {
      QString text = it->what();
      QString head = text.section(".",0,0);
      QString tail = text.section(".", 1);
      html += QString(htmlTemplate).arg(
            "dlg_warning_32", QString("<b>%1.</b> \n%2").arg(head, tail));
      }
    html = QString("<table>%1</table>").arg(html);

    QtWarningDialog msg;
    msg.ui->label->setText(html);
    msg.exec();
    }
}

QtWarningDialog::QtWarningDialog(QWidget *parent)
  : QDialog(parent),
    ui(new Ui::QtWarningDialog)
{
  ui->setupUi(this);
}

QtWarningDialog::~QtWarningDialog()
{
  delete ui;
}



void QtWarningDialog::on_pushButton_clicked()
{
  this->close();
}
