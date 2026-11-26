#include "FileExistenceCache.h"
#include "SNAPQtCommon.h"
#include "ImageIORemote.h"
#include <QFileInfo>
#include <QThreadPool>
#include <iostream>

// How long an answer is used before the file is checked again
static const qint64 RECHECK_INTERVAL_MS = 2000;

FileExistenceCache *FileExistenceCache::instance()
{
  // Never deleted, so that quitting does not wait for a stuck check
  static FileExistenceCache *cache = new FileExistenceCache();
  return cache;
}

FileExistenceCache::FileExistenceCache()
{
  m_Pool = new QThreadPool();
  m_Pool->setMaxThreadCount(4);
}

FileExistenceCache::Status FileExistenceCache::status(const QString &path)
{
  if(path.isEmpty() || IsRemoteImageURL(to_utf8(path)))
    return Exists;

  Entry &e = m_Entries[path];
  if(!e.pending && (!e.checked.isValid() || e.checked.elapsed() > RECHECK_INTERVAL_MS))
    {
    e.pending = true;
    if(m_Debug)
      std::cout << "stat: checking " << to_utf8(path) << std::endl;
    m_Pool->start([this, path]() {
      QElapsedTimer timer;
      timer.start();
      bool exists = QFileInfo::exists(path);
      double elapsed_ms = timer.nsecsElapsed() * 1.0e-6;
      QMetaObject::invokeMethod(this, [this, path, exists, elapsed_ms]() {
        onCheckFinished(path, exists, elapsed_ms);
        }, Qt::QueuedConnection);
      });
    }

  return e.status;
}

void FileExistenceCache::onCheckFinished(QString path, bool exists, double elapsed_ms)
{
  if(m_Debug)
    std::cout << "stat: " << to_utf8(path) << (exists ? " exists" : " is missing")
              << " (" << elapsed_ms << " ms)" << std::endl;

  Entry &e = m_Entries[path];
  Status old_status = e.status;
  e.status = exists ? Exists : Missing;
  e.pending = false;
  e.checked.start();
  // Unknown files are shown as present, so only a change to or from Missing
  // changes what is shown
  if((e.status == Missing) != (old_status == Missing))
    emit statusChanged(path);
}
