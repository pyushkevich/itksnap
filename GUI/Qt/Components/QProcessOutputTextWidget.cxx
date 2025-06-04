#include "QProcessOutputTextWidget.h"
#include <QScrollBar>
#include <QRegularExpression>
#include <QProcess>

QProcessOutputTextWidget::QProcessOutputTextWidget(QWidget *parent)
  : QTextEdit(parent)
{
  setReadOnly(true);
  setDarkTheme(true);
  setLineWrapMode(QTextEdit::NoWrap);
}

void
QProcessOutputTextWidget::appendPlainText(QString text)
{
  moveCursor(QTextCursor::End);
  insertHtml(ansiToHtml(text));
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void
QProcessOutputTextWidget::setDarkTheme(bool dark)
{
  m_darkTheme = dark;

  QString css = R"(
        .ansi-black { color: #000; }
        .ansi-red { color: #e06c75; }
        .ansi-green { color: #98c379; }
        .ansi-yellow { color: #e5c07b; }
        .ansi-blue { color: #61afef; }
        .ansi-magenta { color: #c678dd; }
        .ansi-cyan { color: #56b6c2; }
        .ansi-white { color: #abb2bf; }
        .ansi-bright-black { color: #5c6370; }
        .ansi-bright-red { color: #be5046; }
        .ansi-bright-green { color: #b6e354; }
        .ansi-bright-yellow { color: #dcdcaa; }
        .ansi-bright-blue { color: #61afef; }
        .ansi-bright-magenta { color: #d55fde; }
        .ansi-bright-cyan { color: #00ffff; }
        .ansi-bright-white { color: #ffffff; }
    )";

  if (!dark)
  {
    css.replace("#000", "#000000");
    css.replace("#ffffff", "#000000");
  }

  document()->setDefaultStyleSheet(css);
}

QSize
QProcessOutputTextWidget::sizeHint() const
{
  return QSize(600, 400);
}

void
QProcessOutputTextWidget::appendStdout()
{
  auto *process = dynamic_cast<QProcess *>(sender());
  if (process)
  {
    appendPlainText(QString::fromUtf8(process->readAllStandardOutput()));
  }
}

void
QProcessOutputTextWidget::appendStderr()
{
  auto *process = dynamic_cast<QProcess *>(sender());
  if (process)
  {
    appendPlainText(QString::fromUtf8(process->readAllStandardOutput()));
  }
}

QString
QProcessOutputTextWidget::ansiToHtml(QString input)
{
  static const QRegularExpression ansiRegex(R"(\x1b\[([0-9;]*)m)");
  static const QMap<int, QString> colorMap = {
    { 30, "ansi-black" },        { 31, "ansi-red" },
    { 32, "ansi-green" },        { 33, "ansi-yellow" },
    { 34, "ansi-blue" },         { 35, "ansi-magenta" },
    { 36, "ansi-cyan" },         { 37, "ansi-white" },
    { 90, "ansi-bright-black" }, { 91, "ansi-bright-red" },
    { 92, "ansi-bright-green" }, { 93, "ansi-bright-yellow" },
    { 94, "ansi-bright-blue" },  { 95, "ansi-bright-magenta" },
    { 96, "ansi-bright-cyan" },  { 97, "ansi-bright-white" }
  };

  QString html;
  QString currentClass;
  int     lastPos = 0;

  QRegularExpressionMatchIterator i = ansiRegex.globalMatch(input);

  while (i.hasNext())
  {
    QRegularExpressionMatch match = i.next();
    int                     start = match.capturedStart();
    QString                 codes = match.captured(1);
    QString                 plain = input.mid(lastPos, start - lastPos);
    plain = plain.toHtmlEscaped().replace(QString("\n"),QString("<br>\n"));

    if (!currentClass.isEmpty())
    {
      html += QString("<span class=\"%1\">%2</span>").arg(currentClass, plain);
    }
    else
    {
      html += plain;
    }

    lastPos = match.capturedEnd();

    QStringList parts = codes.split(';');
    for (const QString &part : std::as_const(parts))
    {
      bool ok;
      int  code = part.toInt(&ok);
      if (!ok)
        continue;

      if (code == 0)
      {
        currentClass.clear();
      }
      else if (colorMap.contains(code))
      {
        currentClass = colorMap.value(code);
      }
    }
  }

  QString rest = input.mid(lastPos);
  rest = rest.toHtmlEscaped().replace(QString("\n"),QString("<br>\n"));
  if (!rest.isEmpty())
  {
    if (!currentClass.isEmpty())
      html += QString("<span class=\"%1\">%2</span>").arg(currentClass, rest);
    else
      html += rest;
  }

  return html;
}
