#include "SliceMeshCutRenderer.h"
#include "GenericSliceContextItem.h"
#include <vtkContext2D.h>
#include "GlobalUIModel.h"
#include "Generic3DModel.h"

class SliceMeshCutPipeline : public AbstractModel
{
public:

protected:

};

class SliceMeshCutContextItem: public GenericSliceContextItem
{
public:
  vtkTypeMacro(SliceMeshCutContextItem, GenericSliceContextItem)
  static SliceMeshCutContextItem *New();

  virtual bool Paint(vtkContext2D *painter) override
  {
    // Get the slice model
    GenericSliceModel *slice_model = this->GetModel();

    // Get the 3D model
    Generic3DModel *model_3d = slice_model->GetParentUI()->GetModel3D();

    // Iterate over all the meshes in the 3D model and check the pipeline



    return true;
  }

};


SliceMeshCutRenderer
::SliceMeshCutRenderer()
{

}


void SliceMeshCutRenderer
::AddContextItemsToTiledOverlay(
    vtkAbstractContextItem *parent,
    ImageWrapperBase *)
{
  if(m_Model)
    {
    vtkNew<SliceMeshCutContextItem> ci;
    ci->SetModel(m_Model);
    parent->AddItem(ci);
    }
}
