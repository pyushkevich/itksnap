#ifndef GLOBALWSWIZARDMODEL_H
#define GLOBALWSWIZARDMODEL_H

#include "SNAPCommon.h"
#include "AbstractModel.h"
#include "GlobalState.h"

class GlobalUIModel;
class IRISApplication;

class GlobalWSWizardModel : public AbstractModel
{
public:

  irisITKObjectMacro(GlobalWSWizardModel, AbstractModel)

  void SetParentModel(GlobalUIModel *model);
  irisGetMacro(Parent, GlobalUIModel *)

  /** Called when first displaying the GWS wizard */
  void OnGlobalWSModeEnter();

  /** Cancel segmentation and return to IRIS */
  void OnCancelSegmentation();

  /** Copy segmentation and return to IRIS */
  void OnFinishGWS();

protected:
  GlobalWSWizardModel();
  virtual ~GlobalWSWizardModel() {}

  // Parent model
  GlobalUIModel *m_Parent;
  IRISApplication *m_Driver;
  GlobalState *m_GlobalState;

};

#endif // GLOBALWSWIZARDMODEL_H
