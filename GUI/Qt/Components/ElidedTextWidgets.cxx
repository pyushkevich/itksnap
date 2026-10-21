#include "ElidedTextWidgets.h"
#include <QAbstractItemView>
#include <QFontDatabase>
#include <QHelpEvent>
#include <QPainter>
#include <QStyleOptionComboBox>
#include <QStyleOptionFrame>
#include <QStylePainter>
#include <QTextDocument>
#include <QToolTip>
#include <algorithm>

// Show the full text of an elided widget as its tooltip, ahead of the
// widget's own tooltip if it has one
static bool
showFullTextToolTip(QWidget *widget, QEvent *event, const QString &text)
{
  QString tip = widget->toolTip();
  if (tip.isEmpty() || tip == text)
    tip = text;
  else
    tip = QString("<p>%1</p>%2").arg(
      text.toHtmlEscaped(),
      Qt::mightBeRichText(tip) ? tip : QString("<p>%1</p>").arg(tip.toHtmlEscaped()));

  QToolTip::showText(static_cast<QHelpEvent *>(event)->globalPos(), tip, widget);
  return true;
}

QString
elideText(const QFontMetrics &fm, const QString &text, int width, bool elide_path)
{
  if (fm.horizontalAdvance(text) <= width)
    return text;

  // Split a path into the directory and the filename at the last separator
  qsizetype sep = elide_path ? std::max(text.lastIndexOf('/'), text.lastIndexOf('\\')) : -1;
  if (sep <= 0 || sep == text.size() - 1)
    return fm.elidedText(text, Qt::ElideMiddle, width);

  QString dir = text.left(sep);  // without the separator
  QString name = text.mid(sep);  // with the separator
  int     dir_width = fm.horizontalAdvance(dir);
  int     name_width = fm.horizontalAdvance(name);

  // The filename gets the space first, but the directory keeps at least 40%
  int     dir_budget = std::max(width - name_width, std::min(dir_width, width * 2 / 5));
  QString dir_elided = fm.elidedText(dir, Qt::ElideMiddle, dir_budget);
  int     name_budget = width - fm.horizontalAdvance(dir_elided);
  return dir_elided + fm.elidedText(name, Qt::ElideMiddle, std::max(name_budget, 0));
}

// ----------------------------------------------------------------------------
// ElidedLabel
// ----------------------------------------------------------------------------

ElidedLabel::ElidedLabel(QWidget *parent)
  : QLabel(parent)
{}

void
ElidedLabel::setElidePath(bool value)
{
  m_ElidePath = value;
  update();
}

bool
ElidedLabel::canElide() const
{
  if (wordWrap() || text().contains('\n') || textFormat() == Qt::RichText)
    return false;
  if (textFormat() == Qt::AutoText && Qt::mightBeRichText(text()))
    return false;
  return true;
}

QRect
ElidedLabel::textRect() const
{
  QRect r = contentsRect().adjusted(margin(), margin(), -margin(), -margin());

  // Same indent as QLabel uses
  int ind = indent();
  if (ind < 0 && frameWidth() > 0)
    ind = fontMetrics().horizontalAdvance(QLatin1Char('x')) / 2;
  if (ind > 0)
  {
    Qt::Alignment align = QStyle::visualAlignment(layoutDirection(), alignment());
    if (align & Qt::AlignLeft)
      r.setLeft(r.left() + ind);
    else if (align & Qt::AlignRight)
      r.setRight(r.right() - ind);
  }
  return r;
}

bool
ElidedLabel::isElided() const
{
  return canElide() && fontMetrics().horizontalAdvance(text()) > textRect().width();
}

QSize
ElidedLabel::minimumSizeHint() const
{
  QSize size = QLabel::minimumSizeHint();
  if (canElide())
  {
    int extra = size.width() - fontMetrics().horizontalAdvance(text());
    int width = fontMetrics().horizontalAdvance(QChar(0x2026)) + std::max(extra, 0);
    size.setWidth(std::min(size.width(), width));
  }
  return size;
}

void
ElidedLabel::paintEvent(QPaintEvent *event)
{
  if (!isElided())
  {
    QLabel::paintEvent(event);
    return;
  }

  QPainter painter(this);
  drawFrame(&painter);

  QRect   r = textRect();
  QString elided = elideText(fontMetrics(), text(), r.width(), m_ElidePath);
  style()->drawItemText(&painter,
                        r,
                        QStyle::visualAlignment(layoutDirection(), alignment()),
                        palette(),
                        isEnabled(),
                        elided,
                        foregroundRole());
}

bool
ElidedLabel::event(QEvent *event)
{
  if (event->type() == QEvent::ToolTip && isElided())
    return showFullTextToolTip(this, event, text());
  return QLabel::event(event);
}

// ----------------------------------------------------------------------------
// ElidedLineEdit
// ----------------------------------------------------------------------------

ElidedLineEdit::ElidedLineEdit(QWidget *parent)
  : QLineEdit(parent)
{}

void
ElidedLineEdit::setElidePath(bool value)
{
  m_ElidePath = value;
  update();
}

QRect
ElidedLineEdit::textRect() const
{
  // The same rectangle as QLineEdit::paintEvent, which also leaves a
  // horizontal margin of 2 pixels
  QStyleOptionFrame panel;
  initStyleOption(&panel);
  QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
  return r.marginsRemoved(textMargins()).adjusted(2, 0, -2, 0);
}

bool
ElidedLineEdit::isElided() const
{
  return !hasFocus() && echoMode() == QLineEdit::Normal &&
         fontMetrics().horizontalAdvance(text()) > textRect().width();
}

void
ElidedLineEdit::paintEvent(QPaintEvent *event)
{
  if (!isElided())
  {
    QLineEdit::paintEvent(event);
    return;
  }

  QPainter          painter(this);
  QStyleOptionFrame panel;
  initStyleOption(&panel);
  style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

  QRect   r = textRect();
  QString elided = elideText(fontMetrics(), text(), r.width(), m_ElidePath);
  Qt::Alignment align = QStyle::visualAlignment(layoutDirection(), alignment());
  painter.setClipRect(r);
  painter.setPen(palette().color(QPalette::Text));
  painter.drawText(r, (align & Qt::AlignHorizontal_Mask) | Qt::AlignVCenter, elided);
}

bool
ElidedLineEdit::event(QEvent *event)
{
  if (event->type() == QEvent::ToolTip && isElided())
    return showFullTextToolTip(this, event, text());
  return QLineEdit::event(event);
}

// ----------------------------------------------------------------------------
// ElidedComboBox
// ----------------------------------------------------------------------------

ElidedComboBox::ElidedComboBox(QWidget *parent)
  : QComboBox(parent)
{
  view()->setTextElideMode(Qt::ElideMiddle);
}

int
ElidedComboBox::textWidth() const
{
  // The same rectangle as CE_ComboBoxLabel in QCommonStyle
  QStyleOptionComboBox opt;
  initStyleOption(&opt);
  QRect r = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
  int   width = r.width() - 2;
  if (!opt.currentIcon.isNull())
    width -= opt.iconSize.width() + 4;
  return width;
}

bool
ElidedComboBox::isElided() const
{
  return !isEditable() && fontMetrics().horizontalAdvance(currentText()) > textWidth();
}

void
ElidedComboBox::paintEvent(QPaintEvent *event)
{
  if (!isElided())
  {
    QComboBox::paintEvent(event);
    return;
  }

  // As QComboBox::paintEvent, with the label elided
  QStylePainter painter(this);
  painter.setPen(palette().color(QPalette::Text));

  QStyleOptionComboBox opt;
  initStyleOption(&opt);
  painter.drawComplexControl(QStyle::CC_ComboBox, opt);

  opt.currentText = fontMetrics().elidedText(opt.currentText, Qt::ElideMiddle, textWidth());
  painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

bool
ElidedComboBox::event(QEvent *event)
{
  if (event->type() == QEvent::ToolTip && isElided())
    return showFullTextToolTip(this, event, currentText());
  return QComboBox::event(event);
}

// ----------------------------------------------------------------------------
// ElidedHeaderView
// ----------------------------------------------------------------------------

ElidedHeaderView::ElidedHeaderView(Qt::Orientation orientation, QWidget *parent)
  : QHeaderView(orientation, parent)
{
  setTextElideMode(Qt::ElideMiddle);
}

void
ElidedHeaderView::initStyleOptionForIndex(QStyleOptionHeader *option, int logicalIndex) const
{
  QHeaderView::initStyleOptionForIndex(option, logicalIndex);

  // The width available for the text, as in QCommonStyle's SE_HeaderLabel
  int margin = style()->pixelMetric(QStyle::PM_HeaderMargin, option, this);
  int width = sectionSize(logicalIndex) - 2 * margin;
  if (option->sortIndicator != QStyleOptionHeader::None)
    width -= style()->pixelMetric(QStyle::PM_HeaderMarkSize, option, this) + margin;
  if (!option->icon.isNull())
    width -= style()->pixelMetric(QStyle::PM_SmallIconSize, option, this) + margin;

  QStringList lines = option->text.split('\n');
  for (QString &line : lines)
    line = option->fontMetrics.elidedText(line, textElideMode(), std::max(width, 0));
  option->text = lines.join('\n');

  // The lines fit now, so the style must not elide the text again
  if (auto *opt2 = qstyleoption_cast<QStyleOptionHeaderV2 *>(option))
    opt2->textElideMode = Qt::ElideNone;
}

// ----------------------------------------------------------------------------
// WindowTitleElider
// ----------------------------------------------------------------------------

// Replace %1, %2, ... in format with names. Unlike QString::arg, a name that
// itself contains "%2" is not replaced again.
static QString
substituteNames(const QString &format, const QStringList &names)
{
  QString result;
  for (qsizetype i = 0; i < format.size(); i++)
  {
    if (format[i] == '%' && i + 1 < format.size() && format[i + 1].isDigit())
    {
      int k = format[i + 1].digitValue();
      if (k >= 1 && k <= names.size())
      {
        result += names[k - 1];
        i++;
        continue;
      }
    }
    result += format[i];
  }
  return result;
}

WindowTitleElider::WindowTitleElider(QWidget *window)
  : QObject(window)
  , m_Window(window)
{
  window->installEventFilter(this);
}

void
WindowTitleElider::setTitle(const QString &format, const QStringList &names)
{
  m_Format = format;
  m_Names = names;
  update();
}

QString
WindowTitleElider::fullTitle() const
{
  return substituteNames(m_Format, m_Names);
}

bool
WindowTitleElider::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == m_Window && event->type() == QEvent::Resize)
    update();
  return QObject::eventFilter(watched, event);
}

void
WindowTitleElider::update()
{
  // Room taken by the title bar buttons. On macOS the title is centered, so
  // the buttons on the left take the same room on the right.
#ifdef __APPLE__
  const int reserved = 2 * 90;
#else
  const int reserved = 200;
#endif

  QFontMetrics fm(QFontDatabase::systemFont(QFontDatabase::TitleFont));
  int          budget = m_Window->width() - reserved -
               fm.horizontalAdvance(substituteNames(m_Format, QStringList(m_Names.size(), QString())));

  // Elide the names to share the budget, shortest first, so that a short name
  // keeps its full length and leaves the rest to the longer ones
  QStringList elided = m_Names;
  QList<int>  order;
  for (int i = 0; i < m_Names.size(); i++)
    order.append(i);
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    return fm.horizontalAdvance(m_Names[a]) < fm.horizontalAdvance(m_Names[b]);
  });

  int left = m_Names.size();
  for (int i : order)
  {
    int share = std::max(budget, 0) / left--;
    elided[i] = fm.elidedText(m_Names[i], Qt::ElideMiddle, share);
    budget -= fm.horizontalAdvance(elided[i]);
  }

  m_Window->setWindowTitle(substituteNames(m_Format, elided));
}
