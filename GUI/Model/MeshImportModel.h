#ifndef MESHIMPORTMODEL_H
#define MESHIMPORTMODEL_H

#include "AbstractModel.h"

class MeshImportModel : public AbstractModel
{
public:
  irisITKObjectMacro(MeshImportModel, AbstractModel)


protected:
  MeshImportModel();
  virtual ~MeshImportModel() {}
};

#endif // MESHIMPORTMODEL_H
