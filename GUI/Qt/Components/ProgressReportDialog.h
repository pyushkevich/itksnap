#pragma once

#include <QDialog>
#include "AbstractProgressDelegate.h"

class ProgressReportWidget;
class QtProgressDelegate;
class QTimer;

/**
 * A window-modal dialog hosting an embedded ProgressReportWidget.  Intended
 * for blocking operations (e.g., remote workspace loading) where the user
 * should not interact with the main window.
 *
 * The dialog closes itself automatically when the last task finishes.
 * It has no close button, so the user cannot dismiss it manually.
 *
 * Typical call-site pattern:
 *
 *   AbstractProgressDelegate *saved = driver->GetProgressDelegate();
 *   ProgressReportDialog *dlg = new ProgressReportDialog("Opening...", mainwin);
 *   driver->SetProgressDelegate(dlg->GetDelegate());
 *   dlg->open();                         // window-modal, non-blocking
 *   driver->OpenProject(file, warnings); // processEvents keeps dlg alive
 *   driver->SetProgressDelegate(saved);
 *   // dlg has already closed itself, or will close on the next event loop tick
 */
class ProgressReportDialog : public QDialog
{
  Q_OBJECT
public:
  explicit ProgressReportDialog(const QString &title, QWidget *parent = nullptr);

  /** Delegate to pass to IRISApplication::SetProgressDelegate(). */
  AbstractProgressDelegate *GetDelegate() const;

protected:
  void showEvent(QShowEvent *event) override;

private slots:
  void checkAndShow();

private:
  ProgressReportWidget *m_widget;
  QtProgressDelegate   *m_delegate;
  QTimer               *m_deferTimer;
};
