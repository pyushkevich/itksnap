#include "ProgressReportDialog.h"
#include "ProgressReportWidget.h"
#include "QtProgressDelegate.h"

#include <QLabel>
#include <QVBoxLayout>

ProgressReportDialog::ProgressReportDialog(const QString &title, QWidget *parent)
  : QDialog(parent,
            Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint)
{
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
}

AbstractProgressDelegate *
ProgressReportDialog::GetDelegate() const
{
  return m_delegate;
}
