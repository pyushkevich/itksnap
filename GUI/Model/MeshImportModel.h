#ifndef MESHIMPORTMODEL_H
#define MESHIMPORTMODEL_H

#include "PropertyModel.h"
#include "GuidedMeshIO.h"

class GlobalUIModel;

class MeshImportModel : public AbstractModel
{
public:
  irisITKObjectMacro(MeshImportModel, AbstractModel)

  /** Mode determines file selection behavior
   *    - Single only allows single file selection
   *    - Series allows selecting single/multiple files or a directory */
  enum Mode{ SINGLE = 0, SERIES };

  void SetParentModel(GlobalUIModel *model);
  GlobalUIModel *GetParentModel();

  std::string GetTitle() const;

  /** Mesh file format */
  typedef GuidedMeshIO::FileFormat FileFormat;

  /** Domain for the mesh file format model */
  typedef SimpleItemSetDomain<FileFormat, std::string> FileFormatDomain;

  /** File format for the import */
  irisGenericPropertyAccessMacro(ImportFormat, FileFormat, FileFormatDomain)

  /** Error Mesage property */
  irisSimplePropertyAccessMacro(ErrorMessage, std::string)

  /** Get the name of the history used for mesh import */
  static std::string GetHistoryName() { return "MeshImportHistory"; }

  /** Get File format enum by format name (description) */
  FileFormat GetFileFormatByName(const std::string &formatName) const;

  /** Load file using a GuidedMeshIO */
  void Load(const char *filename, FileFormat format);

  /** Load file with creating blank main image */
  void LoadAndCreateMain(const char *filename, FileFormat format);

  irisGetSetMacro(Mode, Mode)


protected:
  MeshImportModel();
  virtual ~MeshImportModel();

  GlobalUIModel *m_ParentModel;

  // File format for the import
  typedef ConcretePropertyModel<FileFormat, FileFormatDomain> ConcreteFileFormatModel;
  SmartPtr<ConcreteFileFormatModel> m_ImportFormatModel;

  // Error message model
  SmartPtr<ConcreteSimpleStringProperty> m_ErrorMessageModel;

  // Regular expression for file format
  // This is for the file panel to parse and display the list of importable format
  std::map<FileFormat, std::string> m_FormatRegExp;

  Mode m_Mode;
};

#endif // MESHIMPORTMODEL_H
