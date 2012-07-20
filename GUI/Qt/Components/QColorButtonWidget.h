#ifndef QCOLORBUTTONWIDGET_H
#define QCOLORBUTTONWIDGET_H

#include <QWidget>

class QToolButton;

class QColorButtonWidget : public QWidget
{
  Q_OBJECT
public:
  explicit QColorButtonWidget(QWidget *parent = 0);

  Q_PROPERTY(QColor value READ value WRITE setValue NOTIFY valueChanged)

  void setValue(QColor value);

  QColor value() { return m_value; }

signals:
  void valueChanged();

public slots:

  void onButtonPress();

private:
  QToolButton *m_Button;

  QColor m_value;
};

#endif // QCOLORBUTTONWIDGET_H
