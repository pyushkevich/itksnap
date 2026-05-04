#pragma once

#include <cmath>
#include <cstddef>
#include <limits>
#include <string>

/**
 * Abstract interface for reporting named progress tasks. Implementations live
 * in the GUI layer (Qt) and are injected downward into Logic and Model code,
 * keeping those layers free of any toolkit dependency.
 *
 * Usage: wrap each operation in a ProgressTaskGuard, which calls StartTask on
 * construction and CompleteTask on destruction (RAII).
 */
class AbstractProgressDelegate
{
public:
  virtual ~AbstractProgressDelegate() = default;

  /**
   * Begin a new named task. Returns an opaque task ID that must be passed to
   * UpdateProgress and CompleteTask. trackProgress=true requests a determinate
   * progress bar; false requests a spinner.
   *
   * useTimeout=false disables the safety-net auto-removal timer (default true).
   * Set it to false for tasks managed by ProgressTaskGuard, which guarantees
   * CompleteTask is always called; leave it true only for fire-and-forget tasks
   * where a crash might otherwise leave a zombie entry in the overlay.
   */
  virtual std::string StartTask(const char *title, bool trackProgress,
                                bool useTimeout = false) = 0;

  /**
   * Update task progress. percent is in [0, 1]; NaN means indeterminate.
   * Returns false if the user has requested cancellation of this task.
   */
  virtual bool UpdateProgress(const std::string &task_id, double percent) = 0;

  /**
   * Mark the task complete and dismiss it from the UI.
   */
  virtual void CompleteTask(const std::string &task_id) = 0;
};


/**
 * RAII guard that opens a task on construction and closes it on destruction.
 * Also provides a static ProgressCallback suitable for REST/transfer clients
 * that use a (void*, double) callback convention.
 */
class ProgressTaskGuard
{
public:
  ProgressTaskGuard(AbstractProgressDelegate *delegate, const char *title,
                    bool trackProgress = false)
    : m_Delegate(delegate)
  {
    if (m_Delegate)
      m_TaskId = m_Delegate->StartTask(title, trackProgress);
  }

  ~ProgressTaskGuard()
  {
    Complete();
  }

  static void ProgressCallback(void *source, double progress)
  {
    auto *guard = static_cast<ProgressTaskGuard *>(source);
    if (guard && guard->m_Delegate)
      guard->m_Cancelled |= !guard->m_Delegate->UpdateProgress(guard->m_TaskId, progress);
  }

  bool UpdateProgress(double percent)
  {
    if (m_Delegate)
      m_Cancelled |= !m_Delegate->UpdateProgress(m_TaskId, percent);
    return !m_Cancelled;
  }

  bool UpdateProgress(std::size_t done, std::size_t total)
  {
    return UpdateProgress(total > 0 ? static_cast<double>(done) / total
                                    : std::numeric_limits<double>::quiet_NaN());
  }

  bool IsCancelled() const { return m_Cancelled; }

  void Complete()
  {
    if (m_Delegate)
    {
      m_Delegate->CompleteTask(m_TaskId);
      m_Delegate = nullptr;
    }
  }

  ProgressTaskGuard(const ProgressTaskGuard &) = delete;
  ProgressTaskGuard &operator=(const ProgressTaskGuard &) = delete;

private:
  std::string m_TaskId;
  AbstractProgressDelegate *m_Delegate  = nullptr;
  bool                      m_Cancelled = false;
};
