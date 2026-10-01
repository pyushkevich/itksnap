// Test: the 3D view follows Tools > Image Free Rotation (#229).
//
// Free rotation gives the main image and its segmentations an ITK transform T.
// The slice views show the image point T(p) at the reference point p, so a
// structure at x in the image appears at T^-1(x). This test rotates an image
// whose segmentation holds one small cube, off center, and checks, against
// where the slice views show that cube:
//
//   - Pick: a click in the 3D view (a ray in world coordinates) puts the
//     cursor on the cube.
//   - Mesh: the matrices that place the cube's mesh in the 3D view, and a
//     copy of it loaded from a file, put them on the cube.
//   - Outline: the slice views cut the loaded mesh where the cube is shown.
//   - Volume: volume rendering places the cube where the slice views do.
//   - Scalpel: a cut plane through the displayed cube relabels the voxels
//     that the slice views show on its far side.
//
// The rotation is 30 degrees about an oblique axis: an axis-aligned or
// 90-degree rotation could pass by accident. If the transform is ignored,
// the cube is missed by about 7 mm.

#include "GlobalUIModel.h"
#include "Generic3DModel.h"
#include "RegistrationModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "LabelImageWrapper.h"
#include "ImageMeshLayers.h"
#include "MeshWrapperBase.h"
#include "IRISException.h"
#include "ImageIODelegates.h"
#include "SystemInterface.h"
#include "ColorMap.h"
#include "UIReporterDelegates.h"
#include "itkImage.h"
#include "itkImageFileWriter.h"
#include "itkTransform.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkCommand.h"
#include "itksys/SystemTools.hxx"
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkNew.h>
#include <vtkDataArraySelection.h>
#include <vtkPolyDataWriter.h>
#include <vnl/vnl_math.h>
#include <iostream>
#include <cmath>

class DummySystemInfoDelegate : public SystemInfoDelegate
{
public:
  DummySystemInfoDelegate(const char *argv0, std::string dataDir)
    : m_ExecutableName(argv0), m_DataDir(dataDir) {}

  std::string GetApplicationDirectory() override
  { return itksys::SystemTools::GetFilenamePath(m_ExecutableName); }

  std::string GetApplicationFile() override { return m_ExecutableName; }
  std::string GetApplicationPermanentDataLocation() override { return m_DataDir; }
  std::string GetUserDocumentsLocation() override { return m_DataDir; }
  std::string GetTempDirectory() override { return m_DataDir; }
  std::string EncodeServerURL(const std::string &url) override { return url; }

  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}

protected:
  std::string m_ExecutableName, m_DataDir;
};

class DummyColorMapPresetNameSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset preset, bool) override
  { return "Color Map " + std::to_string(static_cast<int>(preset)); }
};

static int n_failures = 0;

static void check(bool condition, const std::string &what)
{
  std::cout << (condition ? "  ok    " : "  FAIL  ") << what << std::endl;
  if (!condition)
    n_failures++;
}

// Image geometry: anisotropic voxels and a direction that permutes and flips
// the axes, so that mixing up index, LPS and RAS coordinates shows up
static const unsigned int N[3] = { 48, 40, 44 };
static const int CUBE_CENTER[3] = { 34, 12, 30 };
static const int CUBE_HALF_WIDTH = 2;

template <class TImage>
static typename TImage::Pointer MakeImage()
{
  auto img = TImage::New();
  typename TImage::RegionType region;
  for (unsigned int d = 0; d < 3; d++)
    region.SetSize(d, N[d]);
  img->SetRegions(region);

  double spacing[3] = { 1.0, 1.25, 0.8 };
  double origin[3] = { -20.0, 15.0, 5.0 };
  img->SetSpacing(spacing);
  img->SetOrigin(origin);

  typename TImage::DirectionType dir;
  dir.Fill(0.0);
  dir(0, 0) = -1.0;
  dir(1, 2) = 1.0;
  dir(2, 1) = 1.0;
  img->SetDirection(dir);

  img->Allocate();
  img->FillBuffer(0);
  return img;
}

template <class TImage>
static void WriteImage(TImage *img, const std::string &fn)
{
  auto writer = itk::ImageFileWriter<TImage>::New();
  writer->SetInput(img);
  writer->SetFileName(fn);
  writer->Update();
}

// LPS physical point to the RAS (NIFTI) world coordinates of the 3D view
static Vector3d LPSToRAS(const itk::Point<double, 3> &p)
{
  return Vector3d(-p[0], -p[1], p[2]);
}

static double Distance(const Vector3d &a, const Vector3d &b)
{
  return (a - b).two_norm();
}

// Index of the segmentation voxel shown at a reference-space voxel, i.e., what
// the slice views show under the cursor
static Vector3i DisplayedSegmentationVoxel(LabelImageWrapper *seg, const Vector3i &ref)
{
  itk::ContinuousIndex<double, 3> ci_ref, ci_seg;
  for (unsigned int d = 0; d < 3; d++)
    ci_ref[d] = ref[d];
  seg->TransformReferenceCIndexToWrappedImageCIndex(ci_ref, ci_seg);

  Vector3i result;
  for (unsigned int d = 0; d < 3; d++)
    result[d] = static_cast<int>(std::floor(ci_seg[d] + 0.5));
  return result;
}

static bool IsOnCube(const Vector3i &idx, int margin)
{
  for (unsigned int d = 0; d < 3; d++)
    if (std::abs(idx[d] - CUBE_CENTER[d]) > CUBE_HALF_WIDTH + margin)
      return false;
  return true;
}

static void Noop(itk::Object *, const itk::EventObject &, void *) {}

static Vector3d BoundsCenter(vtkPolyData *pd)
{
  double b[6];
  pd->GetBounds(b);
  return Vector3d((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2);
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <temp_dir>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string tmpdir = std::string(argv[1]) + "/FreeRotation3DTest";
  itksys::SystemTools::MakeDirectory(tmpdir);

  DummySystemInfoDelegate sidel(argv[0], tmpdir);
  SystemInterface::SetSystemInfoDelegate(&sidel);
  DummyColorMapPresetNameSource color_map_source;
  ColorMap::SetColorMapPresetNameSource(&color_map_source);

  try
  {
    // A main image, and a segmentation with one cube of label 1
    using GreyImage = itk::Image<short, 3>;
    using SegImage = itk::Image<LabelType, 3>;
    auto grey = MakeImage<GreyImage>();
    for (itk::ImageRegionIteratorWithIndex<GreyImage> it(grey, grey->GetBufferedRegion()); !it.IsAtEnd(); ++it)
      it.Set(static_cast<short>(it.GetIndex()[0] + 2 * it.GetIndex()[1] + 3 * it.GetIndex()[2]));

    auto segimg = MakeImage<SegImage>();
    for (itk::ImageRegionIteratorWithIndex<SegImage> it(segimg, segimg->GetBufferedRegion()); !it.IsAtEnd(); ++it)
    {
      Vector3i idx(it.GetIndex()[0], it.GetIndex()[1], it.GetIndex()[2]);
      if (IsOnCube(idx, 0))
        it.Set(1);
    }

    std::string fn_grey = tmpdir + "/main.nii.gz", fn_seg = tmpdir + "/seg.nii.gz";
    WriteImage(grey.GetPointer(), fn_grey);
    WriteImage(segimg.GetPointer(), fn_seg);

    // Load them the way the application does
    SmartPtr<GlobalUIModel> gui = GlobalUIModel::New();
    IRISApplication        *driver = gui->GetDriver();
    IRISWarningList         wl;
    driver->OpenImage(fn_grey.c_str(), MAIN_ROLE, wl);
    driver->OpenImage(fn_seg.c_str(), LABEL_ROLE, wl);

    GenericImageData  *id = driver->GetCurrentImageData();
    ImageWrapperBase  *main = id->GetMain();
    LabelImageWrapper *seg = driver->GetSelectedSegmentationLayer();
    if (!seg)
      throw IRISException("No segmentation is selected after loading");

    // Rotate the image the way Tools > Image Free Rotation does: about the
    // image center, 30 degrees about an oblique axis
    RegistrationModel *reg = gui->GetRegistrationModel();
    reg->SetFreeRotationMode(true);
    reg->Update();
    driver->SetCursorPosition(Vector3i(N[0] / 2, N[1] / 2, N[2] / 2));
    reg->SetCenterOfRotationToCursor();
    Vector3d axis(1.0, 2.0, 3.0);
    axis.normalize();
    reg->ApplyRotation(axis, 30.0 * vnl_math::pi / 180.0);

    // Where the slice views show the cube center, worked out with ITK's own
    // transform: the reference point p with T(p) = the cube center
    const ImageWrapperBase::ITKTransformType *T = seg->GetITKTransform();
    auto                                      T_inv = T->GetInverseTransform();
    itk::Index<3>                             h0 = { { CUBE_CENTER[0], CUBE_CENTER[1], CUBE_CENTER[2] } };
    itk::Point<double, 3>                     x_seg, p_ref;
    seg->GetImageBase()->TransformIndexToPhysicalPoint(h0, x_seg);
    p_ref = T_inv->TransformPoint(x_seg);

    itk::ContinuousIndex<double, 3> e0;
    main->GetReferenceSpace()->TransformPhysicalPointToContinuousIndex(p_ref, e0);
    Vector3i e0_voxel;
    for (unsigned int d = 0; d < 3; d++)
      e0_voxel[d] = static_cast<int>(std::floor(e0[d] + 0.5));

    Vector3d w_cube = LPSToRAS(p_ref);          // displayed cube center, world coordinates
    Vector3d w_unrotated = LPSToRAS(x_seg);     // where it would be if T were ignored

    std::cout << "Setup" << std::endl;
    check(main->GetITKTransform() == T, "the segmentation shares the main image's transform");
    check(DisplayedSegmentationVoxel(seg, e0_voxel) == Vector3i(h0[0], h0[1], h0[2]),
          "the slice views show the cube center at the expected reference voxel");
    check(Distance(w_cube, w_unrotated) > 5.0,
          "the rotation moves the cube far enough for the test to mean something ("
            + std::to_string(Distance(w_cube, w_unrotated)) + " mm)");

    // Pick. Rays come from outside the image from several directions; the
    // first voxel they hit is on the cube's surface. The 3D view updates its
    // model before it renders, so do that here too.
    std::cout << "Pick" << std::endl;
    Generic3DModel *model3d = gui->GetModel3D();
    model3d->Update();
    Vector3d        dirs[3] = { Vector3d(1.0, 0.4, 0.2), Vector3d(-0.3, 1.0, 0.5), Vector3d(0.2, -0.6, -1.0) };
    for (auto u : dirs)
    {
      u.normalize();
      driver->SetCursorPosition(Vector3i(0, 0, 0));
      bool     picked = model3d->PickSegmentationVoxelAlongRay(w_cube + u * 150.0, -u);
      Vector3i shown = DisplayedSegmentationVoxel(seg, driver->GetCursorPosition());
      check(picked && IsOnCube(shown, 1),
            "a ray toward the displayed cube puts the cursor on it (slice views show voxel "
              + std::to_string(shown[0]) + "," + std::to_string(shown[1]) + "," + std::to_string(shown[2]) + ")");
    }

    // A ray that starts inside the cube hits the voxel it starts in, so the
    // cursor must land exactly where the slice views show the cube center
    driver->SetCursorPosition(Vector3i(0, 0, 0));
    bool picked = model3d->PickSegmentationVoxelAlongRay(w_cube, Vector3d(0.0, 0.0, 1.0));
    check(picked && driver->GetCursorPosition() == e0_voxel,
          "a ray from the cube center puts the cursor on the displayed cube center");

    // Mesh. The cube's mesh is in the segmentation's own space; its matrix
    // must move it to where the cube is displayed. A mesh loaded from a file
    // follows the main image: save the same mesh and load it back.
    std::cout << "Mesh" << std::endl;
    auto progress = itk::CStyleCommand::New();
    progress->SetCallback(Noop);
    model3d->UpdateSegmentationMesh(progress);

    ImageMeshLayers *ml = id->GetMeshLayers();
    MeshWrapperBase *mesh = ml->GetMeshForImage(seg->GetUniqueId());
    PolyDataWrapper *pdw = mesh ? mesh->GetMesh(0, 1) : nullptr;
    if (!pdw || !pdw->GetPolyData() || pdw->GetPolyData()->GetNumberOfPoints() == 0)
      throw IRISException("No mesh was generated for label 1");

    Vector3d mesh_center = BoundsCenter(pdw->GetPolyData());
    Vector3d mesh_center_world = affine_transform_point(ml->GetMeshToReferenceNiftiTransform(mesh), mesh_center);
    check(Distance(mesh_center, w_unrotated) < 1.0, "the generated mesh is in the segmentation's own space");
    check(Distance(mesh_center_world, w_cube) < 1.0,
          "the mesh is displayed on the cube (off by " + std::to_string(Distance(mesh_center_world, w_cube))
            + " mm)");

    std::string               fn_mesh = tmpdir + "/cube.vtk";
    vtkNew<vtkPolyDataWriter> mesh_writer;
    mesh_writer->SetInputData(pdw->GetPolyData());
    mesh_writer->SetFileName(fn_mesh.c_str());
    mesh_writer->Write();

    std::vector<std::string> fn_list = { fn_mesh };
    ml->AddLayerFromFiles(fn_list, GuidedMeshIO::FORMAT_VTK);
    MeshWrapperBase *loaded = nullptr;
    for (auto it = ml->GetLayers(); !it.IsAtEnd(); ++it)
      if (it.GetLayer()->IsExternalLoadable())
        loaded = it.GetLayer();
    if (!loaded || !loaded->GetMesh(0, 0) || !loaded->GetMesh(0, 0)->GetPolyData())
      throw IRISException("The mesh file was not loaded");

    auto     loaded_to_ref = ml->GetMeshToReferenceNiftiTransform(loaded);
    Vector3d loaded_center_world =
      affine_transform_point(loaded_to_ref, BoundsCenter(loaded->GetMesh(0, 0)->GetPolyData()));
    check(Distance(loaded_center_world, w_cube) < 1.0,
          "the loaded mesh is displayed on the cube (off by " + std::to_string(Distance(loaded_center_world, w_cube))
            + " mm)");

    // Outline. The slice views outline loaded meshes where the slice plane
    // cuts them. Cut the loaded mesh with an axial plane through the
    // displayed cube center; the cut, drawn through the mesh's matrix as the
    // slice views draw it, must lie in that plane and around the center.
    std::cout << "Outline" << std::endl;
    auto slice_geometry = itk::Image<unsigned char, 3>::New();
    slice_geometry->SetOrigin(p_ref);
    DisplaySliceIndex slice_index(0, DISPLAY_SLICE_MAIN);
    loaded->SetDisplayViewportGeometry(slice_index, slice_geometry);
    vtkPolyData *cut = loaded->GetIntersectionWithSlicePlane(0, 0, slice_index, false, loaded_to_ref);

    vtkIdType n_points = cut ? cut->GetNumberOfPoints() : 0;
    double    max_off_plane = 0.0, max_from_center = 0.0;
    for (vtkIdType i = 0; i < n_points; i++)
    {
      Vector3d x(cut->GetPoint(i));
      Vector3d x_world = affine_transform_point(loaded_to_ref, x);
      max_off_plane = std::max(max_off_plane, std::fabs(x_world[2] - w_cube[2]));
      max_from_center = std::max(max_from_center, Distance(x_world, w_cube));
    }
    check(n_points > 0, "the slice plane through the displayed cube cuts the mesh");
    check(max_off_plane < 1.0e-3,
          "the cut lies in the slice plane (farthest point off it by " + std::to_string(max_off_plane) + " mm)");
    check(max_from_center < 6.0,
          "the cut surrounds the displayed cube center (farthest point "
            + std::to_string(max_from_center) + " mm)");

    // Volume. The main image is volume rendered in VTK coordinates (origin
    // and spacing, no direction). The cube center there must be placed on
    // the displayed cube.
    std::cout << "Volume" << std::endl;
    auto    *vol_image = main->GetDefaultScalarRepresentation()->GetImageBase();
    Vector3d x_vtk;
    for (unsigned int d = 0; d < 3; d++)
      x_vtk[d] = vol_image->GetOrigin()[d] + CUBE_CENTER[d] * vol_image->GetSpacing()[d];
    Vector3d w_volume = affine_transform_point(model3d->GetVolumeToWorldMatrix(main), x_vtk);
    check(Distance(w_volume, w_cube) < 1.0e-4,
          "the volume rendering shows the cube center on the cube (off by "
            + std::to_string(Distance(w_volume, w_cube)) + " mm)");

    // Scalpel. Cut with an oblique plane through the displayed cube center.
    // The cube voxels that the slice views show on the far side of the plane
    // must get the drawing label, and the others keep label 1. Voxels within
    // 1.5 mm of the plane are not checked, since the cut follows the voxel
    // grid. This edits the segmentation, so it comes last.
    std::cout << "Scalpel" << std::endl;
    const LabelType cut_label = 2;
    driver->GetColorLabelTable()->SetColorLabelValid(cut_label, true);
    driver->GetGlobalState()->SetDrawingColorLabel(cut_label);
    Vector3d n_cut(0.3, -0.5, 0.8);
    n_cut.normalize();
    int n_modified = model3d->RelabelSegmentationWithCutPlane(w_cube, n_cut);

    int n_far = 0, n_near = 0, n_wrong = 0;
    for (int i = -CUBE_HALF_WIDTH; i <= CUBE_HALF_WIDTH; i++)
      for (int j = -CUBE_HALF_WIDTH; j <= CUBE_HALF_WIDTH; j++)
        for (int k = -CUBE_HALF_WIDTH; k <= CUBE_HALF_WIDTH; k++)
        {
          itk::Index<3>         h = { { CUBE_CENTER[0] + i, CUBE_CENTER[1] + j, CUBE_CENTER[2] + k } };
          itk::Point<double, 3> x_h;
          seg->GetImageBase()->TransformIndexToPhysicalPoint(h, x_h);
          Vector3d  w_h = LPSToRAS(T_inv->TransformPoint(x_h));
          double    side = dot_product(Vector3d(w_h - w_cube), n_cut);
          LabelType label = seg->GetImage()->GetPixel(h);
          if (side > 1.5)
          {
            n_far++;
            n_wrong += (label != cut_label);
          }
          else if (side < -1.5)
          {
            n_near++;
            n_wrong += (label != 1);
          }
        }
    check(n_modified > 0 && n_far > 0 && n_near > 0 && n_wrong == 0,
          "the cut relabels the displayed cube on the far side of the plane only ("
            + std::to_string(n_wrong) + " of " + std::to_string(n_far + n_near) + " voxels wrong)");
  }
  catch (std::exception &exc)
  {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << (n_failures ? "FAILED: " + std::to_string(n_failures) + " check(s)" : "PASSED") << std::endl;
  return n_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
