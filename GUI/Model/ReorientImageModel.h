#ifndef REORIENTIMAGEMODEL_H
#define REORIENTIMAGEMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>

class GlobalUIModel;

/**
 * @brief The ReorientImageModel class
 * Model for the reorient dialog
 */
class ReorientImageModel : public AbstractModel
{
public:

  // Standard ITK stuff
  irisITKObjectMacro(ReorientImageModel, AbstractModel)

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** A model for the current RAI field */
  irisGetMacro(NewRAICodeModel, AbstractSimpleStringProperty *)

  /** A model for the current RAI field */
  irisGetMacro(CurrentRAICodeModel, AbstractSimpleStringProperty *)

protected:

  // Constructor
  ReorientImageModel();
  virtual ~ReorientImageModel() {}

  // Model for the new RAI code
  SmartPtr<ConcreteSimpleStringProperty> m_NewRAICodeModel;

  // Model for the current RAI code
  SmartPtr<AbstractSimpleStringProperty> m_CurrentRAICodeModel;
  bool GetCurrentRAICodeValue(std::string &value);

  // The parent model
  GlobalUIModel *m_Parent;

};

#endif // REORIENTIMAGEMODEL_H
