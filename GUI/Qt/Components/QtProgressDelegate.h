#pragma once

#include <QObject>
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

  std::string StartTask(const char *title, bool trackProgress) override
  {
    static int counter = 0;
    counter++;
    size_t hash = std::hash<const char *>{}(title) ^ std::hash<int>{}(counter);
    std::string id = std::string("task") + std::to_string(hash);
    m_Widget->startTask(QString::fromStdString(id), QString::fromUtf8(title), trackProgress);
    return id;
  }

  void UpdateProgress(const std::string &task_id, double percent) override
  {
    if (std::isnan(percent))
      m_Widget->updateTaskWithoutProgress(QString::fromStdString(task_id));
    else
      m_Widget->updateTaskProgress(QString::fromStdString(task_id),
                                   static_cast<int>(percent * 100));
  }

  void CompleteTask(const std::string &task_id) override
  {
    m_Widget->finishTask(QString::fromStdString(task_id));
  }

private:
  ProgressReportWidget *m_Widget;
};
