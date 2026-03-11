#ifndef PROGRESSREPORTWIDGET_H
#define PROGRESSREPORTWIDGET_H

#pragma once


#include <QWidget>
#include <QMap>
#include <QTimer>

class QLabel;
class QProgressBar;
class QVBoxLayout;
class QPropertyAnimation;
class QGraphicsOpacityEffect;

// A tiny spinner – you can replace with QMovie/GIF if preferred
class Spinner : public QWidget
{
  Q_OBJECT
public:
  explicit Spinner(QWidget *parent = nullptr);

  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;
  void setComplete();
protected:
  void paintEvent(QPaintEvent *) override;

private:
  int     angle;
  bool    complete;
  QTimer *timer;
};


// Row widget
class ProgressTaskWidget : public QWidget
{
  Q_OBJECT
public:
  explicit ProgressTaskWidget(const QString &label, bool reportsProgress, QWidget *parent = nullptr);

  void setLabel(const QString &txt);
  void showProgress(int pct);
  void showSpinner();
  void markSpinnerComplete();

  void fadeOutAndDelete();
  virtual QSize minimumSizeHint() const override;

private:
  QLabel       *textLabel;
  QProgressBar *progressBar;
  Spinner      *spinner;
};

// Data object describing task state
struct ProgressTask
{
  QString             id;
  QString             label;
  bool                reportsProgress = false;
  int                 progress = -1; // -1 => unknown
  QTimer              spinnerDelayTimer;
  QTimer              autoRemoveTimer;
  QTimer              fadeAndRemoveTimer;
  ProgressTaskWidget *widget = nullptr;
};

class ProgressReportWidget : public QWidget
{
  Q_OBJECT
public:
  explicit ProgressReportWidget(QWidget *parent = nullptr);

  void startTask(const QString &taskId, const QString &label, bool reportsProgress = false);
  void updateTaskLabel(const QString &taskId, const QString &label);
  void updateTaskWithoutProgress(const QString &taskId);
  void updateTaskProgress(const QString &taskId, int percent);
  void finishTask(const QString &taskId);

  void setAutoRemoveSecs(int secs);
  void setSpinnerDelayMs(int ms);

  virtual bool event(QEvent *event) override;
  virtual bool eventFilter(QObject *obj, QEvent *event) override;

private:
  void createTaskUI(ProgressTask &task);
  void removeTaskUI(ProgressTask &task);
  void removeTask(const QString &taskId);
  void repositionToParent();

  QVBoxLayout                  *m_layout;
  QMap<QString, ProgressTask *> m_tasks;

  int m_autoRemoveSecs = 4; // seconds
  int m_spinnerDelayMs = 500;

  // Opacity effect - only set on fade in and fade out
  QGraphicsOpacityEffect *m_opacityEffect = nullptr;

  // Animations for fade in / fade out
  QPropertyAnimation *m_fadeInAnimation, *m_fadeOutAnimation;

  // Fade-in/out for main widget
  void fadeIn();
  void fadeOutIfEmpty();
  void recomputeSizeAndReposition();
};

#endif // PROGRESSREPORTWIDGET_H
