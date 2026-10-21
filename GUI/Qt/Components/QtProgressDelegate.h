#pragma once

#include <QObject>
#include <QCoreApplication>
#include "AbstractProgressDelegate.h"
#include "ProgressReportWidget.h"

/**
 * Qt implementation of AbstractProgressDelegate that forwards to a
 * ProgressReportWidget overlay. Suitable for injection into both Logic
 * (IRISApplication remote downloads) and Model (DLS) layers.
 */
class QtProgressDelegate : public QObject, public AbstractProgressDelegate
{
public:
  explicit QtProgressDelegate(ProgressReportWidget *widget, QObject *parent = nullptr)
    : QObject(parent), m_Widget(widget) {}

  std::string StartTask(const char *title, bool trackProgress,
                        bool useTimeout = false,
                        const std::string &titleArg = std::string()) override
  {
    static int counter = 0;
    counter++;
    size_t hash = std::hash<const char *>{}(title) ^ std::hash<int>{}(counter);
    std::string id = std::string("task") + std::to_string(hash);

    // Titles are marked with SNAP_TRANSLATE_NOOP("ProgressTask", ...)
    QString text = QCoreApplication::translate("ProgressTask", title);
    if (text.contains("%1"))
      text = text.arg(QString::fromStdString(titleArg));

    m_Widget->startTask(QString::fromStdString(id), text, trackProgress, useTimeout);
    return id;
  }

  bool UpdateProgress(const std::string &task_id, double percent) override
  {
    if (std::isnan(percent))
      return m_Widget->updateTaskWithoutProgress(QString::fromStdString(task_id));
    else
      return m_Widget->updateTaskProgress(QString::fromStdString(task_id),
                                          static_cast<int>(percent * 100));
  }

  void CompleteTask(const std::string &task_id) override
  {
    m_Widget->finishTask(QString::fromStdString(task_id));
  }

private:
  ProgressReportWidget *m_Widget;
};
