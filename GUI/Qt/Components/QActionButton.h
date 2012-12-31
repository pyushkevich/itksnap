#ifndef QACTIONBUTTON_H
#define QACTIONBUTTON_H

#include <QPushButton>

class QAction;

class QActionButton : public QPushButton
{
  Q_OBJECT
  Q_PROPERTY(QString action READ action WRITE setAction NOTIFY actionChanged)

public:
  explicit QActionButton(QWidget *parent = 0);
  void setAction(QString action);
  QString action() const;
  
signals:
  void actionChanged(QString);
  
public slots:

  void updateFromAction();

private:

  QAction *m_action;
  
};

#endif // QACTIONBUTTON_H
