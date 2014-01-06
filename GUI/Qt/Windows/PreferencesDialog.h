#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include "SNAPAppearanceSettings.h"

namespace Ui {
class PreferencesDialog;
}

class GlobalPreferencesModel;
class QAbstractButton;
class QStandardItem;

class PreferencesDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit PreferencesDialog(QWidget *parent = 0);
  ~PreferencesDialog();

  void SetModel(GlobalPreferencesModel *model);

  // This method should be used to show the dialog each time!
  void ShowDialog();

  // Goes to a particular page
  void GoToAppearancePage();

private slots:
  void on_listWidget_itemSelectionChanged();

  void on_buttonBox_clicked(QAbstractButton *button);

  void on_btnElementReset_clicked();

  void on_btnElementResetAll_clicked();

  void onModelUpdate(const EventBucket &bucket);

private:
  Ui::PreferencesDialog *ui;

  GlobalPreferencesModel *m_Model;

  // Fill the color map presets
  void UpdateColorMapPresets();

  // Helper methods for building the appearance tree
  QStandardItem *append_appearance_item(
      QStandardItem *parent,
      SNAPAppearanceSettings::UIElements elt,
      const QString &text);

  QStandardItem *append_category_item(
      QStandardItem *parent,
      const QString &text);
};

#endif // PREFERENCESDIALOG_H
