#ifndef TAGLISTWIDGET_H
#define TAGLISTWIDGET_H

#include <QWidget>
#include <QLayout>
#include <QFrame>
#include <QToolButton>
#include <QLineEdit>
#include "SNAPQtCommon.h"



/** A widget used to display a single tag */
class QToolButton;
class QLabel;

class TagWidgetDeleteButton : public QToolButton
{
  Q_OBJECT

public:
  TagWidgetDeleteButton(QWidget *parent);
  virtual QSize sizeHint() const Q_DECL_OVERRIDE;
};

class TagWidget : public QFrame
{
  Q_OBJECT

public:
  TagWidget(QString label, QWidget *parent);
  QString text() const;

signals:
  void deleteButtonClicked();

protected:
  QLayout *m_Layout;
  TagWidgetDeleteButton *m_DeleteButton;
  QLabel *m_Label;
};


class TagListWidget : public QWidget
{
  Q_OBJECT

public:
  TagListWidget(QWidget *parent, QStringList tags = QStringList());

  QStringList tags() const;
  void setTags(const QStringList &tags);
  void clear();

signals:
  void tagsEdited();

public slots:
  void onLineEdit();
  void onLineCompletion();
  void onTagDelete();

protected:
  void addTags(const QStringList &tags);

  QLayout *m_Layout;
  QLineEdit *m_LineEdit;
  QList<TagWidget *> m_Tags;
};

#endif // TAGLISTWIDGET_H
