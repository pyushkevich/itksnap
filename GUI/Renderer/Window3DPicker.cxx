#include "Window3DPicker.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ColorLabelTable.h"
#include "SNAPImageData.h"
#include "ImageRayIntersectionFinder.h"
#include "Generic3DModel.h"
#include "vtkObjectFactory.h"

/** These classes are used internally for m_Ray intersection testing */
class LabelImageHitTester
{
public:
  LabelImageHitTester(const ColorLabelTable *table = NULL)
  {
    m_LabelTable = table;
  }

  int operator()(LabelType label) const
  {
    assert(m_LabelTable);
    if(m_LabelTable->IsColorLabelValid(label))
      {
      const ColorLabel &cl = m_LabelTable->GetColorLabel(label);
      return (cl.IsVisible() && cl.IsVisibleIn3D()) ? 1 : 0;
      }
    else return 0;
  }

private:
  const ColorLabelTable *m_LabelTable;
};

class SnakeImageHitTester
{
public:
  int operator()(float levelSetValue) const
  {
    return levelSetValue <= 0 ? 1 : 0;
  }
};


double Window3DPicker::IntersectWithLine(
    double p1[], double p2[], double tol,
    vtkAssemblyPath *, vtkProp3D *, vtkAbstractMapper3D *)
{
  IRISApplication *app = m_Model->GetParentUI()->GetDriver();
  ImageWrapperBase *main = app->GetCurrentImageData()->GetMain();

  // Transform the points into voxel space
  Vector3d x0 = main->TransformNIFTICoordinatesToVoxelIndex(Vector3d(p1));
  Vector3d x1 = main->TransformNIFTICoordinatesToVoxelIndex(Vector3d(p2));

  Vector3i pos;
  bool result = false;


  // Intersect with the correct wrapper
  if(app->IsSnakeModeActive() && app->GetSNAPImageData()->IsSnakeLoaded())
    {
    typedef ImageRayIntersectionFinder<float, SnakeImageHitTester> RayCasterType;
    RayCasterType caster;

    result = caster.FindIntersection(
          app->GetSNAPImageData()->GetLevelSetImage(), x0, x1 - x0, pos);
    }
  else
    {
    LabelImageWrapper *layer = app->GetCurrentImageData()->GetSegmentation();
    typedef ImageRayIntersectionFinder<LabelType, LabelImageHitTester> Finder;
    Finder finder;
    LabelImageHitTester tester(app->GetColorLabelTable());
    finder.SetHitTester(tester);

    result = finder.FindIntersection(layer->GetImage(), x0, x1 - x0, pos);
    }

  // Apply
  if(result)
      {
      app->SetCursorPosition(to_unsigned_int(pos));
      }

  return 0;
}

vtkCxxRevisionMacro(Window3DPicker, "$Revision: 1.1 $")
vtkStandardNewMacro(Window3DPicker)

