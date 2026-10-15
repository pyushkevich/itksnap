#ifndef SEGMENTATIONAUTOSAVE_H
#define SEGMENTATIONAUTOSAVE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QFutureWatcher>
#include <map>
#include <vector>
#include <memory>

class GlobalUIModel;
class QTimer;
class QLockFile;
class QWidget;

/**
 * Periodically saves a copy of every segmentation layer with unsaved changes
 * to a folder for this session, so that the work can be recovered after a
 * crash of any kind (including segfaults, which the terminate handler in
 * main.cxx doesn't catch).
 *
 * Each session writes to <AppData>/CrashRecovery/Session_<time>_<pid>. The
 * folder holds one .nii.gz per layer, a README.txt that says which image and
 * layer each file came from, and a lock file. A layer is written again only
 * when it changed since the last autosave, and its file is deleted once the
 * user saves the layer or unloads it. The folder is deleted when ITK-SNAP
 * exits normally. A folder whose lock is stale belongs to a session that
 * ended abnormally; ReportRecoveredSessions() renames it to Recovery_<time>
 * and tells the user.
 *
 * The layer image is copied on the GUI thread (cheap, since segmentations are
 * run-length encoded) and the copy is written on a worker thread. The layer
 * itself is never modified: the autosave doesn't change its filename or its
 * "unsaved changes" state.
 */
class SegmentationAutosave : public QObject
{
  Q_OBJECT

  // These are for testing
  Q_PROPERTY(QString sessionDir READ sessionDir WRITE setSessionDir)
  Q_PROPERTY(bool writing READ isWriting)
  Q_PROPERTY(int savedLayerCount READ savedLayerCount)
  Q_PROPERTY(int writeCount READ writeCount)
  Q_PROPERTY(int unsavedLayerCount READ unsavedLayerCount)
  Q_PROPERTY(QStringList savedFiles READ savedFiles)

public:
  SegmentationAutosave(GlobalUIModel *model, QObject *parent);

  /** Waits for a write in progress, then deletes the session folder */
  ~SegmentationAutosave();

  /** Start autosaving every interval_sec seconds (0 means never) */
  void Start(int interval_sec);

  /** The default interval in seconds (the --autosave option overrides it) */
  static constexpr int DefaultInterval = 300;

  /** The folder that holds the session and recovery folders:
   *  SNAP_CRASH_RECOVERY_DIR if set, else <AppData>/CrashRecovery */
  static QString GetRecoveryRoot();

  /**
   * Find the session folders of earlier sessions that ended abnormally. Those
   * holding segmentations are renamed to Recovery_<time> and the user is told
   * where they are; empty ones are deleted.
   */
  static void ReportRecoveredSessions(QWidget *parent);

  /**
   * Called by the crash handler. Writes the latest state of every layer with
   * unsaved changes on the calling thread, then renames the session folder to
   * Recovery_<time> so that the next launch doesn't report it again. Returns
   * the folder, or an empty string if nothing was saved.
   */
  Q_INVOKABLE QString SaveForCrash();

  QString sessionDir() const { return m_SessionDir; }
  void setSessionDir(const QString &dir);
  bool isWriting() const { return m_Watcher->isRunning(); }
  int savedLayerCount() const { return (int) m_Saved.size(); }
  int writeCount() const { return m_WriteCount; }
  int unsavedLayerCount() const;
  QStringList savedFiles() const;

public slots:

  /** Autosave now, without waiting for the application to be idle */
  void autosaveNow();

protected slots:

  void onTimer();
  void onWriteFinished();

protected:

  // A layer to write: a private copy of its image and the file name
  struct Job;

  // Find the layers that changed since the last autosave and copy their
  // images. Files of layers that were saved or unloaded are deleted.
  void CollectJobs(std::vector<Job> &jobs);

  // Write the images (on a worker thread). Returns the ids of the layers
  // that failed to write.
  static QList<unsigned long> WriteJobs(std::vector<Job> jobs);

  // Forget the layers that failed to write and update the README
  void ApplyWriteResult(const QList<unsigned long> &failed);

  // Rename a session folder to Recovery_<time>. Returns the new path, or the
  // old one if the rename failed.
  static QString RenameToRecovery(const QString &session_dir);

  // Whether the user is in the middle of something we shouldn't interrupt
  bool IsBusy() const;

  // Create the session folder and take its lock, if not done yet
  bool PrepareSessionDir();

  // Write the README that describes the files in the folder
  void WriteReadme();

  // Remove the session folder and everything in it
  void RemoveSessionDir();

  GlobalUIModel *m_Model;
  QTimer *m_Timer;
  int m_Interval = 0;

  QString m_SessionDir;
  std::unique_ptr<QLockFile> m_Lock;

  // What was saved for each layer, by the layer's unique id
  struct SavedLayer
  {
    QString filename;
    unsigned long mtime;
    QString nickname;
    QString source;
    QString time;
  };
  std::map<unsigned long, SavedLayer> m_Saved;

  // The worker returns the ids of the layers it failed to write
  QFutureWatcher<QList<unsigned long>> *m_Watcher;
  int m_WriteCount = 0, m_PendingWrites = 0;
};

#endif // SEGMENTATIONAUTOSAVE_H
