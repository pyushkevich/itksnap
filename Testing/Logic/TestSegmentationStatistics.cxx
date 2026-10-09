// Test: segmentation volume and intensity statistics when the segmentation
// and the image layers are on different grids.
//
//   1. Same space: the segmentation is on the main image grid. Statistics must
//      match a direct ITK computation.
//   2. Upsampled: the segmentation is upsampled 2x with nearest neighbor
//      interpolation. Volumes must be unchanged, intensity statistics (now
//      sampled with linear interpolation at sub-voxel positions) close.
//   3. Rotated overlay: a copy of the main image is rotated in physical space
//      and then registered back with the matching transform. Its statistics
//      must match those of the main image.
//
// Usage: TestSegmentationStatistics <testdata_dir> <temp_dir>

#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageIODelegates.h"
#include "SegmentationStatistics.h"
#include "UIReporterDelegates.h"
#include "ImageWrapperBase.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageDuplicator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkResampleImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkAffineTransform.h"
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <map>
#include <cmath>
#include <cstdio>

class SimpleSystemInfoDelegate : public SystemInfoDelegate
{
public:
  SimpleSystemInfoDelegate(const char *argv0, std::string tmp)
    : m_Exe(argv0), m_Temp(tmp) {}

  std::string GetApplicationDirectory() override
    { return itksys::SystemTools::GetFilenamePath(m_Exe); }
  std::string GetApplicationFile() override { return m_Exe; }
  std::string GetApplicationPermanentDataLocation() override { return ".itksnap_test"; }
  std::string GetUserDocumentsLocation() override { return ".itksnap_test"; }
  std::string GetTempDirectory() override { return m_Temp; }
  std::string EncodeServerURL(const std::string &url) override { return url; }

  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}

private:
  std::string m_Exe, m_Temp;
};

class SimpleColorMapSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset p, bool) override
    { return "Preset " + std::to_string(static_cast<int>(p)); }
};

using GrayImage = itk::Image<short, 3>;
using LabelImage = itk::Image<unsigned short, 3>;
using Stats = SegmentationStatistics::EntryMap;

static int n_failures = 0;

static void Check(bool cond, const char *what, LabelType label, double a, double b)
{
  if(!cond)
    {
    printf("  FAIL: label %d: %s (%g vs %g)\n", (int) label, what, a, b);
    n_failures++;
    }
}

template <class TImage>
static typename TImage::Pointer ReadImage(const std::string &fn)
{
  auto reader = itk::ImageFileReader<TImage>::New();
  reader->SetFileName(fn);
  reader->Update();
  return reader->GetOutput();
}

template <class TImage>
static void WriteImage(TImage *img, const std::string &fn)
{
  auto writer = itk::ImageFileWriter<TImage>::New();
  writer->SetInput(img);
  writer->SetFileName(fn);
  writer->Update();
}

// Compute statistics using the currently selected segmentation
static Stats ComputeStats(IRISApplication *app)
{
  SegmentationStatistics stats;
  stats.Compute(app);
  return stats.GetStats();
}

static void PrintStats(const char *title, const Stats &s, size_t column)
{
  printf("%s\n", title);
  printf("  %5s %10s %12s %12s %12s\n", "label", "count", "volume", "mean", "stdev");
  for(auto &it : s)
    printf("  %5d %10lu %12.3f %12.4f %12.4f\n", (int) it.first, it.second.count,
           it.second.volume_mm3, it.second.mean[column], it.second.stdev[column]);
}

// Reference statistics, computed directly on a common grid with ITK
struct RefEntry { unsigned long count = 0; double sum = 0, sumsq = 0; bool border = false; };

static std::map<LabelType, RefEntry> ComputeReference(GrayImage *gray, LabelImage *seg)
{
  std::map<LabelType, RefEntry> ref;
  itk::ImageRegionConstIterator<GrayImage> itg(gray, gray->GetBufferedRegion());
  itk::ImageRegionConstIterator<LabelImage> itl(seg, seg->GetBufferedRegion());
  for(; !itg.IsAtEnd(); ++itg, ++itl)
    {
    RefEntry &e = ref[itl.Get()];
    double v = itg.Get();
    e.count++; e.sum += v; e.sumsq += v * v;
    }
  return ref;
}

// Reference statistics for a segmentation on a different grid than the gray
// image: the gray image is sampled with linear interpolation at the center of
// each segmentation voxel. Samples outside of the gray image are skipped.
static std::map<LabelType, RefEntry> ComputeInterpolatedReference(GrayImage *gray, LabelImage *seg)
{
  using Interp = itk::LinearInterpolateImageFunction<GrayImage, double>;
  auto interp = Interp::New();
  interp->SetInputImage(gray);

  std::map<LabelType, RefEntry> ref;
  itk::ImageRegionConstIteratorWithIndex<LabelImage> itl(seg, seg->GetBufferedRegion());
  for(; !itl.IsAtEnd(); ++itl)
    {
    itk::Point<double, 3> p;
    seg->TransformIndexToPhysicalPoint(itl.GetIndex(), p);
    itk::ContinuousIndex<double, 3> cix;
    gray->TransformPhysicalPointToContinuousIndex(p, cix);
    if(!interp->IsInsideBuffer(cix))
      continue;
    double v = interp->EvaluateAtContinuousIndex(cix);
    RefEntry &e = ref[itl.Get()];
    e.count++; e.sum += v; e.sumsq += v * v;

    // Within half a voxel of the image boundary, ITK clamps to the edge
    // voxel while ITK-SNAP down-weights the sample. Flag such labels.
    auto size = gray->GetBufferedRegion().GetSize();
    for(int d = 0; d < 3; d++)
      if(cix[d] < 0.0 || cix[d] > size[d] - 1.0)
        e.border = true;
    }
  return ref;
}

// Upsample a label image by an integer factor using nearest neighbor. Every
// new voxel lies inside exactly one original voxel, so volumes are preserved.
static LabelImage::Pointer UpsampleLabels(LabelImage *seg, int factor)
{
  auto spacing = seg->GetSpacing();
  auto size = seg->GetBufferedRegion().GetSize();
  LabelImage::SpacingType new_spacing;
  LabelImage::SizeType new_size;
  itk::Vector<double, 3> shift;
  for(int d = 0; d < 3; d++)
    {
    new_spacing[d] = spacing[d] / factor;
    new_size[d] = size[d] * factor;
    shift[d] = 0.5 * (new_spacing[d] - spacing[d]);
    }

  using NN = itk::NearestNeighborInterpolateImageFunction<LabelImage, double>;
  auto resample = itk::ResampleImageFilter<LabelImage, LabelImage>::New();
  resample->SetInput(seg);
  resample->SetInterpolator(NN::New());
  resample->SetOutputSpacing(new_spacing);
  resample->SetOutputOrigin(seg->GetOrigin() + seg->GetDirection() * shift);
  resample->SetOutputDirection(seg->GetDirection());
  resample->SetSize(new_size);
  resample->SetDefaultPixelValue(0);
  resample->Update();
  return resample->GetOutput();
}

// Rotation by angle (radians) about an axis, using Rodrigues' formula
static vnl_matrix_fixed<double, 3, 3> RotationMatrix(vnl_vector_fixed<double, 3> axis, double angle)
{
  axis.normalize();
  vnl_matrix_fixed<double, 3, 3> K, I;
  K.fill(0.0); I.set_identity();
  K(0,1) = -axis[2]; K(0,2) =  axis[1];
  K(1,0) =  axis[2]; K(1,2) = -axis[0];
  K(2,0) = -axis[1]; K(2,1) =  axis[0];
  return I + K * sin(angle) + K * K * (1.0 - cos(angle));
}

int main(int argc, char *argv[])
{
  if(argc < 3)
    {
    std::cerr << "Usage: " << argv[0] << " <testdata_dir> <temp_dir>" << std::endl;
    return 1;
    }

  std::string data_dir = argv[1];
  std::string temp_dir = argv[2];
  itksys::SystemTools::MakeDirectory(temp_dir);
  std::string fn_main = data_dir + "/MRIcrop-orig.gipl.gz";
  std::string fn_seg = data_dir + "/MRIcrop-seg.gipl.gz";
  std::string fn_seg_up = temp_dir + "/TestSegStats_seg_upsampled.nii.gz";
  std::string fn_rotated = temp_dir + "/TestSegStats_rotated.mha";

  SimpleSystemInfoDelegate sidel(argv[0], temp_dir);
  SystemInterface::SetSystemInfoDelegate(&sidel);
  SimpleColorMapSource cmSource;
  ColorMap::SetColorMapPresetNameSource(&cmSource);

  try
    {
    GrayImage::Pointer gray = ReadImage<GrayImage>(fn_main);
    LabelImage::Pointer seg = ReadImage<LabelImage>(fn_seg);

    // ── 1. Segmentation on the main image grid ───────────────────────────
    IRISApplication::Pointer app = IRISApplication::New();
    IRISWarningList wl;
    app->OpenImage(fn_main.c_str(), MAIN_ROLE, wl);
    app->OpenImage(fn_seg.c_str(), LABEL_ROLE, wl);

    Stats s_same = ComputeStats(app);
    PrintStats("Same-space segmentation:", s_same, 0);

    auto ref = ComputeReference(gray, seg);
    auto sp = seg->GetSpacing();
    double vox_vol = sp[0] * sp[1] * sp[2];
    Check(s_same.size() == ref.size(), "number of labels", 0, s_same.size(), ref.size());
    for(auto &it : ref)
      {
      LabelType l = it.first;
      const RefEntry &r = it.second;
      const auto &e = s_same[l];
      double mean = r.sum / r.count;
      double sd = sqrt((r.sumsq - r.sum * mean) / (r.count - 1));
      Check(e.count == r.count, "voxel count", l, e.count, r.count);
      Check(fabs(e.volume_mm3 - r.count * vox_vol) < 1e-6, "volume", l, e.volume_mm3, r.count * vox_vol);
      Check(fabs(e.mean[0] - mean) < 1e-6 * (1 + fabs(mean)), "mean", l, e.mean[0], mean);
      Check(fabs(e.stdev[0] - sd) < 1e-6 * (1 + sd), "stdev", l, e.stdev[0], sd);
      }

    // ── 3. Overlay rotated in physical space, then registered back ───────
    // Rotate the image about its center c: a voxel at main physical point p
    // is moved to c + R(p - c). The transform from reference space to the
    // overlay's space is the same map, so after registration every
    // segmentation voxel samples the overlay at the same voxel as the main
    // image (but through the oblique, interpolating code path).
    vnl_vector_fixed<double, 3> axis(1.0, 2.0, 3.0);
    vnl_matrix_fixed<double, 3, 3> R = RotationMatrix(axis, 5.0 * vnl_math::pi / 180.0);

    itk::ContinuousIndex<double, 3> cix_center;
    auto gsize = gray->GetBufferedRegion().GetSize();
    for(int d = 0; d < 3; d++)
      cix_center[d] = 0.5 * (gsize[d] - 1.0);
    itk::Point<double, 3> center;
    gray->TransformContinuousIndexToPhysicalPoint(cix_center, center);

    auto dup = itk::ImageDuplicator<GrayImage>::New();
    dup->SetInputImage(gray);
    dup->Update();
    GrayImage::Pointer rotated = dup->GetOutput();

    GrayImage::DirectionType dir(R * gray->GetDirection().GetVnlMatrix().as_matrix());
    vnl_vector_fixed<double, 3> o = gray->GetOrigin().GetVnlVector() - center.GetVnlVector();
    vnl_vector_fixed<double, 3> o_rot = R * o + center.GetVnlVector();
    rotated->SetDirection(dir);
    rotated->SetOrigin(GrayImage::PointType(o_rot.data_block()));
    WriteImage<GrayImage>(rotated, fn_rotated);

    app->OpenImage(fn_rotated.c_str(), OVERLAY_ROLE, wl);
    ImageWrapperBase *overlay = app->GetCurrentImageData()->GetLastOverlay();

    using AffineType = itk::AffineTransform<double, 3>;
    auto affine = AffineType::New();
    AffineType::MatrixType M;
    M.GetVnlMatrix().copy_in(R.data_block());
    affine->SetCenter(center);
    affine->SetMatrix(M);
    overlay->SetITKTransform(overlay->GetReferenceSpace(), affine);

    Stats s_rot = ComputeStats(app);
    PrintStats("Rotated and registered overlay:", s_rot, 1);
    for(auto &it : s_same)
      {
      LabelType l = it.first;
      const auto &e = s_rot[l];
      double m0 = e.mean[0], m1 = e.mean[1], sd0 = e.stdev[0], sd1 = e.stdev[1];
      Check(fabs(m1 - m0) < 1e-3 * (1 + fabs(m0)), "rotated overlay mean", l, m1, m0);
      Check(fabs(sd1 - sd0) < 1e-3 * (1 + sd0), "rotated overlay stdev", l, sd1, sd0);
      }

    // Without the registration, the overlay is misaligned and should give
    // noticeably different values, or the check above proves nothing
    overlay->SetITKTransform(overlay->GetReferenceSpace(), nullptr);
    Stats s_misreg = ComputeStats(app);
    double max_diff = 0.0;
    for(auto &it : s_misreg)
      max_diff = std::max(max_diff, fabs(it.second.mean[1] - it.second.mean[0]));
    printf("Unregistered rotated overlay: largest difference in mean = %g\n", max_diff);
    Check(max_diff > 1.0, "unregistered overlay mean should differ", 0, max_diff, 1.0);

    // ── 2. Upsampled segmentation ─────────────────────────────────────────
    LabelImage::Pointer seg_up = UpsampleLabels(seg, 2);
    WriteImage<LabelImage>(seg_up, fn_seg_up);

    app = IRISApplication::New();
    app->OpenImage(fn_main.c_str(), MAIN_ROLE, wl);
    app->OpenImage(fn_seg_up.c_str(), LABEL_ROLE, wl);
    Stats s_up = ComputeStats(app);
    PrintStats("Upsampled segmentation:", s_up, 0);

    Check(s_up.size() == s_same.size(), "number of labels", 0, s_up.size(), s_same.size());
    for(auto &it : s_same)
      {
      LabelType l = it.first;
      const auto &a = it.second;
      const auto &b = s_up[l];
      Check(b.count == 8 * a.count, "upsampled voxel count", l, b.count, 8 * a.count);
      Check(fabs(b.volume_mm3 - a.volume_mm3) < 1e-6 * a.volume_mm3, "upsampled volume",
            l, b.volume_mm3, a.volume_mm3);

      // Each new voxel center is a quarter voxel from the center of an
      // original voxel, so intensities are blended with the neighbors, which
      // shifts the mean of small labels next to brighter or darker tissue.
      // It should still be close to the same-space value.
      Check(fabs(b.mean[0] - a.mean[0]) < 0.75 * a.stdev[0], "upsampled mean similar",
            l, b.mean[0], a.mean[0]);
      Check(fabs(b.stdev[0] - a.stdev[0]) < 0.25 * a.stdev[0], "upsampled stdev similar",
            l, b.stdev[0], a.stdev[0]);
      }

    // Compare to direct linear interpolation at the upsampled voxel centers.
    // This should agree closely, except for labels that touch the image
    // boundary, where samples get fractional weights.
    auto ref_up = ComputeInterpolatedReference(gray, seg_up);
    for(auto &it : ref_up)
      {
      LabelType l = it.first;
      const RefEntry &r = it.second;
      const auto &e = s_up[l];
      double mean = r.sum / r.count;
      double sd = sqrt((r.sumsq - r.sum * mean) / (r.count - 1));
      double tol = r.border ? 1e-2 : 1e-4;
      printf("  label %d: interpolated reference mean %.4f, stdev %.4f%s\n", (int) l, mean, sd,
             r.border ? " (touches image boundary)" : "");
      Check(fabs(e.mean[0] - mean) < tol * (1 + fabs(mean)), "upsampled mean vs reference",
            l, e.mean[0], mean);
      Check(fabs(e.stdev[0] - sd) < tol * (1 + sd), "upsampled stdev vs reference",
            l, e.stdev[0], sd);
      }
    }
  catch(std::exception &exc)
    {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return 1;
    }

  if(n_failures)
    {
    printf("%d check(s) failed\n", n_failures);
    return 1;
    }

  printf("All checks passed\n");
  return 0;
}
