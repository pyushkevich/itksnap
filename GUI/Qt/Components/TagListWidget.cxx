#include "TagListWidget.h"
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include "QtFlowLayout.h"

TagWidget::TagWidget(QString label, QWidget *parent)
  : QFrame(parent)
{
  // Create the layout
  m_Layout = new QHBoxLayout(this);
  m_Layout->setSpacing(2);
  m_Layout->setContentsMargins(1,1,3,1);
  this->setLayout(m_Layout);

  // Create the delete button with the cross icon
  m_DeleteButton = new TagWidgetDeleteButton(this);
  m_Label = new QLabel(label, this);

  // Add to layout
  m_Layout->addWidget(m_DeleteButton);
  m_Layout->addWidget(m_Label);

  // Set the stylesheet
  this->setStyleSheet("TagWidget {border: 1px solid gray; border-radius: 4px; }\n"
                      "QToolButton { margin: 1px; padding: 1px; }");

  // Fire delete button signal
  connect(m_DeleteButton, SIGNAL(clicked(bool)), this, SIGNAL(deleteButtonClicked()));
}

QString TagWidget::text() const
{
  return m_Label->text();
}


TagListWidget::TagListWidget(QWidget *parent, QStringList tags)
  : QWidget(parent)
{
  m_Layout = new QtFlowLayout(this, 0, 2, 2);
  this->setLayout(m_Layout);

  m_LineEdit = new QLineEdit(this);
  m_Layout->addWidget(m_LineEdit);

  this->addTags(tags);

  connect(m_LineEdit, SIGNAL(textEdited(QString)), this, SLOT(onLineEdit()));
  connect(m_LineEdit, SIGNAL(returnPressed()), this, SLOT(onLineCompletion()));
  connect(m_LineEdit, SIGNAL(editingFinished()), this, SLOT(onLineCompletion()));
}

QStringList TagListWidget::tags() const
{
  QStringList tags;
  foreach(TagWidget *tag, m_Tags)
    tags.push_back(tag->text());
  return tags;
}

void TagListWidget::setTags(const QStringList &tags)
{
  this->clear();
  this->addTags(tags);
}

void TagListWidget::clear()
{
  foreach(TagWidget *tag, m_Tags)
    {
    m_Layout->removeWidget(tag);
    delete tag;
    }
  m_Tags.clear();
}

void TagListWidget::onLineEdit()
{
  // Any text before a comma is removed and added as a tag
  QStringList parts = m_LineEdit->text().split(",");
  QString tail = parts.back(); parts.pop_back();
  this->addTags(parts);

  m_LineEdit->setText(tail);

  emit tagsEdited();
}

void TagListWidget::onLineCompletion()
{
  QStringList parts = m_LineEdit->text().split(",");
  this->addTags(parts);
  m_LineEdit->clear();

  emit tagsEdited();
}

void TagListWidget::onTagDelete()
{
  TagWidget *tw = dynamic_cast<TagWidget *>(this->sender());
  if(tw)
    {
    m_Tags.removeAll(tw);
    m_Layout->removeWidget(tw);
    delete tw;
    }

  emit tagsEdited();
}

void TagListWidget::addTags(const QStringList &tags)
{
  QStringList curr = this->tags();
  foreach(const QString &tag, tags)
    {
    if(tag.length() > 0 && !curr.contains(tag))
      {
      TagWidget *tw = new TagWidget(tag, this);
      m_Layout->addWidget(tw);
      connect(tw, SIGNAL(deleteButtonClicked()), this, SLOT(onTagDelete()));
      m_Tags.push_back(tw);
      curr.push_back(tag);
      }
    }
}

TagWidgetDeleteButton::TagWidgetDeleteButton(QWidget *parent)
  : QToolButton(parent)
{
  this->setIcon(QIcon(":/root/icons8_delete_sign_16.png"));
  this->setAutoRaise(true);
}

QSize TagWidgetDeleteButton::sizeHint() const
{
  QSize icon_size = this->iconSize();
  icon_size.setHeight(icon_size.height() + 2);
  icon_size.setWidth(icon_size.width() + 2);
  return icon_size;
}
