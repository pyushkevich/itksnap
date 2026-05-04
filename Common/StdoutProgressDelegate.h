#pragma once
#include "AbstractProgressDelegate.h"
#include <string>
#include <vector>

/**
 * AbstractProgressDelegate implementation that renders named task progress to
 * stdout.  In a TTY context each active task overwrites the current line using
 * CR; in a non-TTY context (piped / redirected) it falls back to simple
 * append-only lines.
 *
 * Multiple concurrent tasks are supported: only the most recently started
 * incomplete task is rendered in the active line; older tasks finish silently
 * on the TTY (their completion is still printed in non-TTY mode).
 */
class StdoutProgressDelegate : public AbstractProgressDelegate
{
public:
  StdoutProgressDelegate();

  std::string StartTask(const char *title, bool trackProgress,
                        bool useTimeout = false) override;
  bool UpdateProgress(const std::string &task_id, double percent) override;
  void CompleteTask(const std::string &task_id) override;

private:
  struct Task
  {
    std::string title;
    bool        reportsProgress = false;
    double      progress        = -1.0;  // -1 = not yet reported
    int         spinnerFrame    = 0;
  };

  int  m_NextId         = 0;
  bool m_IsTTY          = false;
  bool m_HasPartialLine = false;

  std::vector<std::pair<std::string, Task>> m_Tasks;

  Task *findTask(const std::string &id);
  void  renderTask(const Task &t);
  void  renderCurrentTask();
};
