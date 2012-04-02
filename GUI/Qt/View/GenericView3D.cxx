#include "GenericView3D.h"
#include "Generic3DModel.h"

GenericView3D::GenericView3D(QWidget *parent) :
    SNAPQGLWidget(parent)
{
}

void GenericView3D::SetModel(Generic3DModel *model)
{
  this->m_Model = model;
}
