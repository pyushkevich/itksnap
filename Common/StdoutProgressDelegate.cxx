#include "StdoutProgressDelegate.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#  include <io.h>
#  define SNAP_ISATTY() (_isatty(_fileno(stdout)) != 0)
#else
#  include <unistd.h>
#  define SNAP_ISATTY() (isatty(STDOUT_FILENO) != 0)
#endif

static const char kSpinnerFrames[] = "|/-\\";
static const int  kBarWidth        = 28;
static const int  kTitleWidth      = 30;

StdoutProgressDelegate::StdoutProgressDelegate()
  : m_IsTTY(SNAP_ISATTY())
{
}

std::string StdoutProgressDelegate::StartTask(const char *title, bool trackProgress)
{
  std::string id = std::to_string(m_NextId++);

  Task t;
  t.title           = title ? title : "";
  t.reportsProgress = trackProgress;
  m_Tasks.push_back({id, std::move(t)});

  if (!m_IsTTY)
  {
    std::printf("  %s...\n", m_Tasks.back().second.title.c_str());
    std::fflush(stdout);
  }

  return id;
}

auto StdoutProgressDelegate::findTask(const std::string &id) -> Task *
{
  for (auto &p : m_Tasks)
    if (p.first == id)
      return &p.second;
  return nullptr;
}

void StdoutProgressDelegate::renderTask(const Task &t)
{
  if (t.reportsProgress && t.progress >= 0.0 && !std::isnan(t.progress))
  {
    int  pct    = std::min(100, static_cast<int>(t.progress * 100));
    int  filled = pct * kBarWidth / 100;

    char bar[kBarWidth + 1];
    std::memset(bar, ' ', kBarWidth);
    std::memset(bar, '#', filled);
    bar[kBarWidth] = '\0';

    std::printf("  %-*.*s: %3d%% [%s]",
                kTitleWidth, kTitleWidth, t.title.c_str(), pct, bar);
  }
  else
  {
    std::printf("  %-*.*s  %c",
                kTitleWidth, kTitleWidth, t.title.c_str(),
                kSpinnerFrames[t.spinnerFrame & 3]);
  }
}

void StdoutProgressDelegate::renderCurrentTask()
{
  if (m_Tasks.empty())
    return;
  std::printf("\r");
  renderTask(m_Tasks.back().second);
  std::printf("\033[K");  // clear to end of line (safe: only called when m_IsTTY)
  std::fflush(stdout);
  m_HasPartialLine = true;
}

void StdoutProgressDelegate::UpdateProgress(const std::string &task_id, double percent)
{
  Task *t = findTask(task_id);
  if (!t)
    return;

  t->progress = percent;
  ++t->spinnerFrame;

  if (!m_IsTTY)
    return;

  if (!m_Tasks.empty() && m_Tasks.back().first == task_id)
    renderCurrentTask();
}

void StdoutProgressDelegate::CompleteTask(const std::string &task_id)
{
  Task *t = findTask(task_id);
  if (!t)
    return;

  if (m_IsTTY)
  {
    if (!m_Tasks.empty() && m_Tasks.back().first == task_id)
    {
      std::printf("\r  %-*.*s: done\033[K\n",
                  kTitleWidth, kTitleWidth, t->title.c_str());
      std::fflush(stdout);
      m_HasPartialLine = false;
    }
    // Non-current tasks complete silently on a TTY.
  }
  else
  {
    std::printf("  %s: done\n", t->title.c_str());
    std::fflush(stdout);
  }

  m_Tasks.erase(
    std::remove_if(m_Tasks.begin(), m_Tasks.end(),
      [&task_id](const auto &p) { return p.first == task_id; }),
    m_Tasks.end());

  // Redraw the new active task if one remains.
  if (m_IsTTY && !m_Tasks.empty())
    renderCurrentTask();
}
