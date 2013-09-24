#ifndef COLLAPSABLEGROUPBOX_H
#define COLLAPSABLEGROUPBOX_H

#include <QWidget>

namespace Ui {
class CollapsableGroupBox;
}

class CollapsableGroupBox : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString title READ title WRITE setTitle USER true)
  
public:
  explicit CollapsableGroupBox(QWidget *parent = 0);
  ~CollapsableGroupBox();

  QString title() const;
  void setTitle(QString title);

  void addWidget(QWidget *widget);

public slots:
  void collapse(bool flag);
  
private:
  Ui::CollapsableGroupBox *ui;
};

#endif // COLLAPSABLEGROUPBOX_H
