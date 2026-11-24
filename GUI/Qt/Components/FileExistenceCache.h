#ifndef FILEEXISTENCECACHE_H
#define FILEEXISTENCECACHE_H

#include <QObject>
#include <QHash>
#include <QElapsedTimer>

class QThreadPool;

/**
 * Checks in the background whether the files in the recent history still
 * exist, so that missing ones can be marked (#314). Checking never blocks the
 * GUI thread: status() returns the last known answer and starts a new check,
 * and statusChanged() is emitted when an answer changes. A file on a hung
 * network mount stays Unknown, and holds at most one worker thread.
 *
 * The object is a singleton that lives on the GUI thread.
 */
class FileExistenceCache : public QObject
{
  Q_OBJECT

public:

  enum Status { Unknown, Exists, Missing };

  static FileExistenceCache *instance();

  /**
   * The last known status of a file, starting a check if none is running and
   * the last answer is older than two seconds. Remote URLs and empty names
   * are reported as existing, since checking them needs the network.
   */
  Status status(const QString &path);

  bool isMissing(const QString &path) { return status(path) == Missing; }

  // Print a line for each check, with its time (--debug-stat)
  void setDebug(bool debug) { m_Debug = debug; }

signals:

  // Emitted when a file is found missing, or found again
  void statusChanged(QString path);

private:

  FileExistenceCache();

  void onCheckFinished(QString path, bool exists, double elapsed_ms);

  struct Entry
  {
    Status status = Unknown;
    bool pending = false;
    QElapsedTimer checked;
  };

  QHash<QString, Entry> m_Entries;

  // Never deleted: its destructor would wait for checks stuck on a dead mount
  QThreadPool *m_Pool;

  bool m_Debug = false;
};

#endif // FILEEXISTENCECACHE_H
