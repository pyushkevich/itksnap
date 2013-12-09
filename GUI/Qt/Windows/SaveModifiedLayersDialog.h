#ifndef SAVEMODIFIEDLAYERSDIALOG_H
#define SAVEMODIFIEDLAYERSDIALOG_H

#include <QDialog>

namespace Ui {
class SaveModifiedLayersDialog;
}

class SaveModifiedLayersModel;
class EventBucket;
class GlobalUIModel;

class SaveModifiedLayersDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit SaveModifiedLayersDialog(QWidget *parent = 0);
  ~SaveModifiedLayersDialog();

  void SetModel(SaveModifiedLayersModel *model);

  /**
   * Flags affecting the dialog's behavior
   */
  enum PromptOption
  {
    NoOption = 0x00,
    DiscardDisabled = 0x01
  };

  Q_DECLARE_FLAGS(PromptOptions, PromptOption)

  void SetOptions(PromptOptions opts);

  /**
   * This static method prompts user to save unsaved changes to all the layers,
   * and return true if the user indicated that he/she wishes to proceed with
   * the subsequent operation
   */
  static bool PromptForUnsavedChanges(
      GlobalUIModel *model, PromptOptions opts = NoOption, QWidget *parent = NULL);

private slots:

  void onModelUpdate(const EventBucket &bucket);
  
private:
  Ui::SaveModifiedLayersDialog *ui;

  SaveModifiedLayersModel *m_Model;

  void UpdateUnsavedTable();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SaveModifiedLayersDialog::PromptOptions)

#endif // SAVEMODIFIEDLAYERSDIALOG_H
