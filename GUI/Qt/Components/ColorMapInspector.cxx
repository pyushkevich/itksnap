#include "ColorMapInspector.h"
#include "ui_ColorMapInspector.h"
#include "ColorMapModel.h"
#include "QtReporterDelegates.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "QtWidgetActivator.h"
#include <QColorDialog>
#include <QInputDialog>
#include "SNAPQtCommon.h"

ColorMapInspector::ColorMapInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ColorMapInspector)
{
  ui->setupUi(this);

  m_PresetsUpdating = false;

  // Create the viewport reporter
  m_ColorMapBoxViewportReporter = QtViewportReporter::New();
  m_ColorMapBoxViewportReporter->SetClientWidget(ui->boxColorMap);

  // Connect the interactor on the colormap box to this object
  ui->boxColorMap->GetDelegate()->SetInspectorWidget(this);
}

ColorMapInspector::~ColorMapInspector()
{
  delete ui;
}

void ColorMapInspector::SetModel(ColorMapModel *model)
{
  // Set the model
  m_Model = model;

  // Pass the model to the colormap box
  ui->boxColorMap->SetModel(model);

  // Connect the viewport reporter
  model->SetViewportReporter(m_ColorMapBoxViewportReporter);

  // Listen to model update events
  connectITK(m_Model, ModelUpdateEvent());
  connectITK(m_Model, ColorMapModel::PresetUpdateEvent());

  // Connect widgets to the corresponding sub-models
  makeCoupling(ui->inControlX, m_Model->GetMovingControlPositionModel());
  makeCoupling(ui->inControlOpacity, m_Model->GetMovingControlOpacityModel());
  makeCoupling(ui->inControlIndex, m_Model->GetMovingControlIndexModel());

  // Connect radio button groups to corresponding enums
  makeRadioGroupCoupling(ui->grpRadioCont,
                         m_Model->GetMovingControlContinuityModel());

  makeRadioGroupCoupling(ui->grpRadioSide,
                         m_Model->GetMovingControlSideModel());

  // Set up activations
  activateOnFlag(this, m_Model,
                 ColorMapModel::UIF_LAYER_ACTIVE);

  activateOnFlag(ui->inControlX, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT);
  activateOnFlag(ui->inControlIndex, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED);
  activateOnFlag(ui->btnControlColor, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED);
  activateOnFlag(ui->btnDeleteControl, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT);
  activateOnFlag(ui->inControlOpacity, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED);
  activateOnFlag(ui->btnCont, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT);
  activateOnFlag(ui->btnDiscont, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT);
  activateOnFlag(ui->btnLeft, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_DISCONTINUOUS);
  activateOnFlag(ui->btnRight, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_DISCONTINUOUS);
  activateOnNotFlag(ui->btnAddPreset, m_Model,
                    ColorMapModel::UIF_PRESET_SELECTED);
  activateOnFlag(ui->btnDelPreset, m_Model,
                 ColorMapModel::UIF_USER_PRESET_SELECTED);

  // Populate the presets
  PopulatePresets();
}

void ColorMapInspector::onModelUpdate(const EventBucket &b)
{
  // Get the model to update itself
  m_Model->Update();

  if(b.HasEvent(ColorMapModel::PresetUpdateEvent()))
    {
    this->PopulatePresets();
    }
  if(b.HasEvent(ModelUpdateEvent()))
    {
    // We don't have a coupling for the color button, so we assign the
    // color to it directly
    Vector3ui rgb = to_unsigned_int(m_Model->GetSelectedColor() * 255.0);
    ui->btnControlColor->setIcon(CreateColorBoxIcon(25,25,rgb));
    }

  // Also we don't have a coupling for the current preset, so we will
  // update it too
  if(m_Model->GetLayer())
    {
    m_PresetsUpdating = true;
    std::string sel = m_Model->GetSelectedPreset();
    int index = ui->inPreset->findText(sel.c_str());
    ui->inPreset->setCurrentIndex(index);
    m_PresetsUpdating = false;
    }
}

void ColorMapInspector::PromptUserForColor()
{
  Vector3d clr = m_Model->GetSelectedColor();
  QColor qc; qc.setRgbF(clr(0), clr(1), clr(2));
  QColor color = QColorDialog::getColor(qc, this);
  m_Model->SetSelectedColor(Vector3d(color.redF(), color.greenF(), color.blueF()));
}

void ColorMapInspector::on_btnControlColor_clicked()
{
  this->PromptUserForColor();
}

// TODO: this should be done using a combo box coupling!
void ColorMapInspector::PopulatePresets()
{
  // Add the presets to the list
  m_PresetsUpdating = true;

  // Populte the combo box (for now, we don't have a coupling for this)
  PopulateColorMapPresetCombo(ui->inPreset, m_Model);

  // Done updating
  m_PresetsUpdating = false;
}

void ColorMapInspector::on_inPreset_currentIndexChanged(int index)
{
  // Set the preset
  if(!m_PresetsUpdating)
    m_Model->SelectPreset(to_utf8(ui->inPreset->itemText(index)));
}

void ColorMapInspector::on_btnAddPreset_clicked()
{
  // Prompt the user for input
  bool ok;
  QString input = QInputDialog::getText(
        this, tr("Preset Name"),
        tr("Enter the name for the new preset:"),
        QLineEdit::Normal, "", &ok);

  if(ok && !input.isEmpty())
    {
    m_Model->SaveAsPreset(to_utf8(input));
    }
}

void ColorMapInspector::on_btnDelPreset_clicked()
{
  int sel = ui->inPreset->currentIndex();
  QString seltext = ui->inPreset->itemText(sel);
  m_Model->DeletePreset(to_utf8(seltext));
}


void ColorMapInspector::on_btnDeleteControl_clicked()
{
  m_Model->DeleteSelectedControl();
}
