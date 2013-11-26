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
    double pt1[], double pt2[], double tol,
    vtkAssemblyPath *path, vtkProp3D *prop, vtkAbstractMapper3D *mapper)
{
  // This method will be called once for every prop in the Renderer. We only
  // need to compute the intersection once. So we cache the input point
  // parameters
  static Vector3d p1_cache(0.0), p2_cache(0.0);
  Vector3d p1(pt1), p2(pt2);

  if(p1 == p1_cache && p2 == p2_cache)
    return VTK_DOUBLE_MAX;


  IRISApplication *app = m_Model->GetParentUI()->GetDriver();
  ImageWrapperBase *main = app->GetCurrentImageData()->GetMain();

  // Transform the points into voxel space
  Vector3d x0 = main->TransformNIFTICoordinatesToVoxelIndex(Vector3d(p1));
  Vector3d x1 = main->TransformNIFTICoordinatesToVoxelIndex(Vector3d(p2));

  Vector3i pos;
  int result = -1;
  double t = VTK_DOUBLE_MAX;

  // Intersect with the correct wrapper
  if(app->IsSnakeModeActive() && app->GetSNAPImageData()->IsSnakeLoaded())
    {
    typedef ImageRayIntersectionFinder<float, SnakeImageHitTester> RayCasterType;
    RayCasterType caster;

    result = caster.FindIntersection(
          app->GetSNAPImageData()->GetSnake()->GetImage(), x0, x1 - x0, pos);
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
  if(result > 0)
    {
    // Set the cursor position at the right place
    m_PickSuccessful = true;
    m_PickPosition = pos;

    // Find the closest point along the ray and return it
    t = dot_product(x0-x1, x0-to_double(pos)) / dot_product(x0-x1, x0-x1);
    }
  else
    {
    m_PickSuccessful = false;
    }

  p1_cache = p1;
  p2_cache = p2;

  return t;
}

Window3DPicker::Window3DPicker()
{
  m_PickSuccessful = false;
  m_Model = NULL;
}

vtkCxxRevisionMacro(Window3DPicker, "$Revision: 1.1 $")
vtkStandardNewMacro(Window3DPicker)

