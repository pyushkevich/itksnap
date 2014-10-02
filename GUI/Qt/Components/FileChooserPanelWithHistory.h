#ifndef FILECHOOSERPANELWITHHISTORY_H
#define FILECHOOSERPANELWITHHISTORY_H

#include <QWidget>
#include <QMap>

namespace Ui {
  class FileChooserPanelWithHistory;
}

class GlobalUIModel;

/**
 * This component is used by different ITK-SNAP dialogs to prompt the
 * user for a filename, while presenting a history drop-down button and
 * a browse button.
 */
class FileChooserPanelWithHistory : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(QString absoluteFilename READ absoluteFilename NOTIFY absoluteFilenameChanged)
  Q_PROPERTY(QString activeFormat READ activeFormat NOTIFY activeFormatChanged)
  Q_PROPERTY(QString errorTest READ errorText WRITE setErrorText)

public:
  explicit FileChooserPanelWithHistory(QWidget *parent = 0);
  ~FileChooserPanelWithHistory();

  // Initialize the panel for opening a file
  void initializeForOpenFile(
      GlobalUIModel *model,
      const QString &labelText,
      const QString &historyCategory,
      const QString &filePattern,
      const QString &initialFile = QString(),
      const QString &activeFormat = QString());

  // Initialize the panel for opening a file
  void initializeForSaveFile(
      GlobalUIModel *model,
      const QString &labelText,
      const QString &historyCategory,
      const QString &filePattern,
      bool force_extension,
      const QString &initialFile = QString(),
      const QString &activeFormat = QString());

  // Add a button to the button panel (customization)
  void addButton(QWidget *button);

  // Provide a custom "oracle" to determine file format from filename
  void setCustomFormatOracle(QObject *target, const char *slot);

  // Get the filename selected
  QString absoluteFilename() const;

  // Get teh active format
  QString activeFormat() const;

  QString errorText() const;
  void setErrorText(const QString &text);

signals:

  void absoluteFilenameChanged(QString text);
  void activeFormatChanged(QString text);

public slots:

  // This slot should be envoked when the container dialog
  // accepts the filename. The path of the filename will be
  // recorded so the next time we invoke this dialog, the
  // same path will be used
  void onFilenameAccept();

  void setActiveFormat(QString format);

private slots:
  void on_btnBrowse_clicked();

  void onHistorySelection();

  void on_inFilename_textChanged(const QString &text);

protected:

  // Populate the history
  void populateHistory();

  // Fix the extension of the file in the dialog
  QString fixExtension() const;

  virtual bool eventFilter(QObject *obj, QEvent *ev);

  // Update filename based on history button or browse
  void updateFilename(QString filename);

private:
  Ui::FileChooserPanelWithHistory *ui;

  bool m_openMode;
  bool m_directoryMode;
  bool m_forceExtension;

  QString m_historyCategory;
  QString m_filePattern;
  QString m_workingDir;
  QString m_defaultFormat;

  QMap<QString, QStringList> m_Filter;

  GlobalUIModel *m_Model;
  void parseFilters(const QString &activeFormat = QString());
  void highlightFilename();

  // Get the filename selected
  QString absoluteFilenameKeepExtension() const;

  QObject *m_oracleTarget;
  const char *m_oracleSlot;
  QString guessFormat(const QString &text);
};

#endif // FILECHOOSERPANELWITHHISTORY_H
