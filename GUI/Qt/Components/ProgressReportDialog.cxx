#include "ProgressReportDialog.h"
#include "ProgressReportWidget.h"
#include "QtProgressDelegate.h"

#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

ProgressReportDialog::ProgressReportDialog(const QString &title, QWidget *parent)
  : QDialog(parent,
            Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint)
{
  qDebug() << "ProgressReportDialog::ProgressReportDialog";
  setWindowTitle(title);
  setWindowModality(Qt::WindowModal);

  setStyleSheet(R"(
    ProgressReportDialog {
        background-color: rgb(40, 40, 40);
    }
    QLabel#titleLabel {
        color: rgba(255, 255, 255, 200);
        font-weight: bold;
        font-size: 13px;
    }
  )");

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(10);

  auto *titleLabel = new QLabel(title, this);
  titleLabel->setObjectName("titleLabel");
  layout->addWidget(titleLabel);

  // Non-floating widget embedded directly in the dialog layout
  m_widget = new ProgressReportWidget(this, /*floating=*/false);
  m_widget->setAutoRemoveSecs(1);
  layout->addWidget(m_widget);

  setFixedWidth(380);
  setSizeGripEnabled(false);

  m_delegate = new QtProgressDelegate(m_widget, this);

  // Close the dialog as soon as the last task disappears
  connect(m_widget, &ProgressReportWidget::tasksEmpty, this, &ProgressReportDialog::accept);

  // Timer used to defer showing when another modal dialog is already open
  m_deferTimer = new QTimer(this);
  m_deferTimer->setInterval(100);
  m_deferTimer->setSingleShot(false);
  connect(m_deferTimer, &QTimer::timeout, this, &ProgressReportDialog::checkAndShow);
}

void ProgressReportDialog::showEvent(QShowEvent *event)
{
  QDialog::showEvent(event);

  // If another modal dialog is already open, hide immediately and poll
  // until it closes before appearing.  This prevents us from blocking
  // dialogs such as SSH password prompts that appear during loading.
  QWidget *other = QApplication::activeModalWidget();
  if(other && other != this)
    {
    QTimer::singleShot(0, this, &ProgressReportDialog::hide);
    if(!m_deferTimer->isActive())
      m_deferTimer->start();
    }
}

void ProgressReportDialog::checkAndShow()
{
  // Abort if all tasks are already done
  if(!m_widget->hasTasks())
    {
    m_deferTimer->stop();
    return;
    }

  // Show only when no other modal dialog is blocking
  QWidget *other = QApplication::activeModalWidget();
  if(!other || other == this)
    {
    m_deferTimer->stop();
    show();
    }
}

AbstractProgressDelegate *
ProgressReportDialog::GetDelegate() const
{
  return m_delegate;
}
