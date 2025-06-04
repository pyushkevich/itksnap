#ifndef QPROCESSOUTPUTTEXTWIDGET_H
#define QPROCESSOUTPUTTEXTWIDGET_H

#include <QTextEdit>

class QProcessOutputTextWidget : public QTextEdit
{
  Q_OBJECT
public:
  explicit QProcessOutputTextWidget(QWidget *parent = nullptr);

  void appendPlainText(QString text);
  void  setDarkTheme(bool dark);
  QSize sizeHint() const override;

public slots:
  void appendStdout();
  void appendStderr();

private:
  QString ansiToHtml(QString input);
  bool    m_darkTheme = true;
};

#endif // QPROCESSOUTPUTTEXTWIDGET_H
