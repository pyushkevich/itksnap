#ifndef MESHEXPORTMODEL_H
#define MESHEXPORTMODEL_H

#include "PropertyModel.h"
#include "ColorLabelPropertyModel.h"
#include "GuidedMeshIO.h"

class GlobalUIModel;

class MeshExportModel : public AbstractModel
{
public:
  irisITKObjectMacro(MeshExportModel, AbstractModel)

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_LABEL_SELECTION_ACTIVE
    };

  void SetParentModel(GlobalUIModel *parent);

  bool CheckState(UIState flag);

  /** Selected save mode */
  enum SaveMode { SAVE_SINGLE_LABEL, SAVE_MULTIPLE_FILES, SAVE_SCENE };

  /** Mesh file format */
  typedef GuidedMeshIO::FileFormat FileFormat;

  /** Domain for the mesh file format model */
  typedef SimpleItemSetDomain<FileFormat, std::string> FileFormatDomain;

  /** Model governing the selected save mode */
  irisSimplePropertyAccessMacro(SaveMode, SaveMode)

  /** Model for selecting the color label */
  irisGenericPropertyAccessMacro(ExportedLabel, LabelType, ColorLabelItemSetDomain)

  /** Filename for the export */
  irisSimplePropertyAccessMacro(ExportFileName, std::string)

  /** File format for the export */
  irisGenericPropertyAccessMacro(ExportFormat, FileFormat, FileFormatDomain)

  /** Get the parent model */
  irisGetMacro(ParentModel, GlobalUIModel *)

  /** Get the name of the history used for mesh export */
  static std::string GetHistoryName() { return "SegmentationMesh"; }

  /** This method is called when the dialog is opened */
  void OnDialogOpen();

  /** Perform the actual save */
  void SaveMesh();

  /** Get the file format corresponding to a name */
  FileFormat GetFileFormatByName(const std::string &name) const;

protected:

  MeshExportModel();
  virtual ~MeshExportModel() {}

  // Parent model
  GlobalUIModel *m_ParentModel;

  // Model for the way meshes are saved
  typedef AbstractPropertyModel<SaveMode, TrivialDomain> AbstractSaveModeModel;
  SmartPtr<AbstractSaveModeModel> m_SaveModeModel;

  bool GetSaveModeValue(SaveMode &value);
  void SetSaveModeValue(SaveMode value);

  // Save mode (cached)
  SaveMode m_SaveMode;

  // Color label used
  SmartPtr<ConcreteColorLabelPropertyModel> m_ExportedLabelModel;

  // Filename for the export
  std::string m_ExportFileName;

  // Model wrapping around the filename
  SmartPtr<AbstractSimpleStringProperty> m_ExportFileNameModel;
  bool GetExportFileNameValue(std::string &value);
  void SetExportFileNameValue(std::string value);

  // File format for the export
  typedef ConcretePropertyModel<FileFormat, FileFormatDomain> ConcreteFileFormatModel;
  SmartPtr<ConcreteFileFormatModel> m_ExportFormatModel;

  // Update the domain of the format model based on the current state of the save
  // mode. This is because only some formats are supported in save modes
  void UpdateFormatDomain();

  // Regular expressions for file formats
  std::map<FileFormat, std::string> m_FormatRegExp;

};

#endif // MESHEXPORTMODEL_H
