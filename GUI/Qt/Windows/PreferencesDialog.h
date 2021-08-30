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

  enum PreferencesDialogPage {General = 0, SliceView, Appearance, Rendering3D, Tools};

  // Goes to a particular page
  void GoToPage(enum PreferencesDialogPage page);

private slots:
  void on_listWidget_itemSelectionChanged();

  void on_buttonBox_clicked(QAbstractButton *button);

  void on_btnElementReset_clicked();

  void on_btnElementResetAll_clicked();

  void onModelUpdate(const EventBucket &bucket);

  void on_btnASC_toggled(bool check);
  void on_btnACS_toggled(bool check);
  void on_btnCAS_toggled(bool check);
  void on_btnCSA_toggled(bool check);
  void on_btnSAC_toggled(bool check);
  void on_btnSCA_toggled(bool check);

  void on_radio_axial_lr_toggled(bool check);
  void on_radio_sagittal_ap_toggled(bool check);

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

  enum CutPlane{Axial = 0, Coronal, Sagittal};

  std::map<std::string, CutPlane> m_StringToCutPlaneMap
  {
    std::make_pair("axial", Axial),
    std::make_pair("coronal", Coronal),
    std::make_pair("sagittal", Sagittal)
  };

  struct SliceLayoutPixmapPath
  {
    SliceLayoutPixmapPath(){};

    void SetDialog(PreferencesDialog *dialog) { m_dialog = dialog; }

    // generate path based on current state
    QString GetPath()
    {
      QString ret(m_header.c_str());

      if (m_cp == Axial || m_cp == Coronal)
        {
          if (m_cp == Axial)
            ret.append("axial_");
          else
            ret.append("coronal_");

          if (m_dialog->m_IsPatientsRightShownLeft)
            ret.append("rl");
          else
            ret.append("lr");
        }
      else if (m_cp == Sagittal)
        {
          ret.append("sagittal_");

          if (m_dialog->m_IsAnteriorShownLeft)
            ret.append("ap");
          else
            ret.append("pa");
        }

      ret.append(m_suffix.c_str());

      return ret;
    }

    PreferencesDialog *m_dialog;
    const std::string m_header = ":/root/layout_";
    CutPlane m_cp = Axial;
    const std::string m_suffix = ".png";
  };


  // Fill out view by cut plane
  void setOutViewCutPlane(enum CutPlane topleft, enum CutPlane topright, enum CutPlane bottomright);

  // Update out views pixmaps
  void UpdateOutViewPixmaps();

  bool m_IsPatientsRightShownLeft = true;
  bool m_IsAnteriorShownLeft = true;

  SliceLayoutPixmapPath m_SliceLayoutPixmapPaths[3];

};

#endif // PREFERENCESDIALOG_H
