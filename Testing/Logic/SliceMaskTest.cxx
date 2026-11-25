/**
 * Test: the mask of obliquely sliced images (#193).
 *
 * When an image is sliced obliquely (Image Free Rotation, registered or
 * differently oriented overlays), the slice covers the whole viewport, and
 * its pixels outside of the image hold 0. The slicer marks them in a mask,
 * and the coloring filters make them transparent, so that they are no longer
 * shown with the color for intensity 0.
 *
 * The test image has intensities from 0 up, with a block of zero voxels
 * inside it. It is sliced through AdaptiveSlicingPipeline:
 *
 *   - orthogonally: the mask is 255 everywhere
 *   - obliquely, rotated 30 degrees in a viewport larger than the image:
 *     the mask is 0 outside of the image and 255 inside, including the zero
 *     block, which keeps the color for 0
 *   - the scalar and RGB coloring filters make the pixels outside of the
 *     image transparent, and only those
 *
 * Usage: SliceMaskTest
 */

#include "AdaptiveSlicingPipeline.h"
#include "ColorLookupTable.h"
#include "ImageCoordinateTransform.h"
#include "LookupTableIntensityMappingFilter.h"
#include "RGBALookupTableIntensityMappingFilter.h"

#include "itkEuler3DTransform.h"
#include "itkImage.h"
#include "itkMath.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkRGBAPixel.h"

#include <cmath>
#include <iostream>

static int g_Failures = 0;

#define CHECK(cond, msg)                                                       \
  if (!(cond))                                                                 \
  {                                                                            \
    std::cerr << "FAILED: " << msg << std::endl;                               \
    ++g_Failures;                                                              \
  }                                                                            \
  else                                                                         \
    std::cout << "  ok    " << msg << std::endl;

using ImageType = itk::Image<short, 3>;
using SliceType = itk::Image<short, 2>;
using PipelineType = AdaptiveSlicingPipeline<ImageType, SliceType, ImageType>;
using MaskType = PipelineType::MaskImageType;
using RGBAPixelType = itk::RGBAPixel<unsigned char>;
using DisplaySliceType = itk::Image<RGBAPixelType, 2>;

// The image: 32 x 32 x 32 voxels of 1mm at the origin. The block of voxels
// with x and y in [8, 13] is zero; the other voxels are 100 or more
static const int  N = 32;
static const int  ZERO_LO = 8, ZERO_HI = 13;

static ImageType::Pointer
MakeImage()
{
  auto img = ImageType::New();
  ImageType::RegionType region;
  region.SetSize({ { N, N, N } });
  img->SetRegions(region);
  img->Allocate();
  for (itk::ImageRegionIteratorWithIndex<ImageType> it(img, region); !it.IsAtEnd(); ++it)
  {
    auto idx = it.GetIndex();
    bool in_block = idx[0] >= ZERO_LO && idx[0] <= ZERO_HI && idx[1] >= ZERO_LO && idx[1] <= ZERO_HI;
    it.Set(in_block ? 0 : static_cast<short>(100 + (idx[0] + idx[1]) % 50));
  }
  return img;
}

int
main(int, char *[])
{
  auto image = MakeImage();

  // ---------------------------------------------------------------------------
  std::cout << "Orthogonal slicing" << std::endl;
  auto pipeline = PipelineType::New();
  pipeline->SetInput(image);

  auto ortho = ImageCoordinateTransform::New();
  ortho->SetTransform(Vector3i(1, 2, 3), Vector3ui(N, N, N));
  pipeline->SetOrthogonalTransform(ortho);
  pipeline->SetSliceIndex(itk::Index<3>({ { N / 2, N / 2, N / 2 } }));
  pipeline->SetUseOrthogonalSlicing(true);
  pipeline->Update();

  MaskType *mask = pipeline->GetMaskOutput();
  CHECK(mask->GetBufferedRegion() == pipeline->GetOutput()->GetBufferedRegion(),
        "the mask has the size of the slice");
  int n_not_full = 0;
  for (itk::ImageRegionConstIterator<MaskType> it(mask, mask->GetBufferedRegion()); !it.IsAtEnd(); ++it)
    n_not_full += (it.Get() != 255);
  CHECK(n_not_full == 0, "the mask is 255 everywhere (" << n_not_full << " pixels are not)");

  // ---------------------------------------------------------------------------
  std::cout << "Oblique slicing" << std::endl;

  // A 64 x 64 viewport of 1mm pixels, in the plane z = 16, larger than the image
  auto reference = itk::ImageBase<3>::New();
  itk::ImageBase<3>::RegionType ref_region;
  ref_region.SetSize({ { 64, 64, 1 } });
  reference->SetRegions(ref_region);
  itk::ImageBase<3>::PointType ref_origin;
  ref_origin[0] = -16.0;
  ref_origin[1] = -16.0;
  ref_origin[2] = 16.0;
  reference->SetOrigin(ref_origin);

  // Rotate 30 degrees about the z axis through the center of the image
  auto rotation = itk::Euler3DTransform<double>::New();
  itk::Euler3DTransform<double>::InputPointType center;
  center[0] = 15.5;
  center[1] = 15.5;
  center[2] = 16.0;
  rotation->SetCenter(center);
  rotation->SetRotation(0.0, 0.0, 30.0 * itk::Math::pi / 180.0);

  pipeline->SetObliqueTransform(rotation);
  pipeline->SetObliqueReferenceImage(reference);
  pipeline->SetUseNearestNeighbor(false);
  pipeline->SetUseOrthogonalSlicing(false);
  pipeline->Update();

  SliceType *slice = pipeline->GetOutput();
  mask = pipeline->GetMaskOutput();
  CHECK(mask->GetBufferedRegion().GetSize() == slice->GetBufferedRegion().GetSize() &&
          slice->GetBufferedRegion().GetSize()[0] == 64,
        "the mask has the size of the 64 x 64 viewport");

  // Where each viewport pixel samples the image, in voxel units (1mm voxels at
  // the origin). Pixels within a voxel of the edge of the image are not checked
  auto where = [&](const SliceType::IndexType &idx, double &x, double &y) {
    itk::Point<double, 3> p;
    p[0] = ref_origin[0] + idx[0];
    p[1] = ref_origin[1] + idx[1];
    p[2] = ref_origin[2];
    auto q = rotation->TransformPoint(p);
    x = q[0];
    y = q[1];
  };

  int n_inside = 0, n_outside = 0, n_zero_inside = 0, n_wrong = 0;
  for (itk::ImageRegionConstIteratorWithIndex<SliceType> it(slice, slice->GetBufferedRegion());
       !it.IsAtEnd();
       ++it)
  {
    double x, y;
    where(it.GetIndex(), x, y);
    unsigned char m = mask->GetPixel(it.GetIndex());
    bool inside = x > 0.5 && x < N - 1.5 && y > 0.5 && y < N - 1.5;
    bool outside = x < -1.5 || x > N + 0.5 || y < -1.5 || y > N + 0.5;
    bool in_block = x > ZERO_LO + 0.5 && x < ZERO_HI - 0.5 && y > ZERO_LO + 0.5 && y < ZERO_HI - 0.5;
    if (inside)
    {
      n_inside++;
      n_wrong += (m != 255);
      if (in_block)
      {
        n_zero_inside++;
        n_wrong += (it.Get() != 0);
      }
    }
    else if (outside)
    {
      n_outside++;
      n_wrong += (m != 0 || it.Get() != 0);
    }
  }
  CHECK(n_inside > 0 && n_outside > 0 && n_zero_inside > 0,
        "the viewport has pixels inside (" << n_inside << "), outside (" << n_outside
                                           << ") and in the zero block (" << n_zero_inside << ")");
  CHECK(n_wrong == 0,
        "the mask is 255 inside, also on zero voxels, and 0 outside (" << n_wrong << " pixels wrong)");

  // ---------------------------------------------------------------------------
  std::cout << "Coloring" << std::endl;

  // A color for each intensity from 0 to 200, with the color for 0 not black
  using LUTType = ColorLookupTable<short, RGBAPixelType>;
  auto lut = LUTType::New();
  lut->Initialize(0, 200, 0.0, 1.0);
  for (unsigned int i = 0; i < lut->GetSize(); i++)
  {
    RGBAPixelType c;
    c[0] = static_cast<unsigned char>(i);
    c[1] = 20;
    c[2] = 30;
    c[3] = 255;
    lut->SetLUTValue(i, c);
  }

  using IntensityFilterType = LookupTableIntensityMappingFilter<SliceType, DisplaySliceType>;
  auto unmasked = IntensityFilterType::New();
  unmasked->SetInput(slice);
  unmasked->SetLookupTable(lut);
  unmasked->Update();

  auto masked = IntensityFilterType::New();
  masked->SetInput(slice);
  masked->SetLookupTable(lut);
  masked->SetMaskInput(mask);
  masked->Update();

  using RGBLUTType = ColorLookupTable<short, unsigned char>;
  auto rgb_lut = RGBLUTType::New();
  rgb_lut->Initialize(0, 200, 0.0, 1.0);
  for (unsigned int i = 0; i < rgb_lut->GetSize(); i++)
    rgb_lut->SetLUTValue(i, static_cast<unsigned char>(i));

  using RGBFilterType = RGBALookupTableIntensityMappingFilter<SliceType>;
  auto rgb = RGBFilterType::New();
  rgb->SetMappingModeToThreeChannelRGB();
  for (unsigned int j = 0; j < 3; j++)
    rgb->SetInput(j, slice);
  rgb->SetLookupTable(rgb_lut);
  rgb->SetMaskInput(mask);
  rgb->Update();

  int n_unmasked_opaque_outside = 0, n_wrong_masked = 0, n_wrong_rgb = 0, n_zero_color = 0;
  for (itk::ImageRegionConstIteratorWithIndex<SliceType> it(slice, slice->GetBufferedRegion());
       !it.IsAtEnd();
       ++it)
  {
    auto idx = it.GetIndex();
    RGBAPixelType c_unmasked = unmasked->GetOutput()->GetPixel(idx);
    RGBAPixelType c_masked = masked->GetOutput()->GetPixel(idx);
    RGBAPixelType c_rgb = rgb->GetOutput()->GetPixel(idx);
    if (mask->GetPixel(idx))
    {
      // Inside: the color from the lookup table, opaque
      n_wrong_masked += (c_masked != c_unmasked || c_masked[3] != 255);
      n_wrong_rgb += (c_rgb[3] != 255);
      n_zero_color += (it.Get() == 0 && c_masked[0] == 0 && c_masked[1] == 20 && c_masked[3] == 255);
    }
    else
    {
      // Outside: fully transparent with the mask, the color for 0 without it
      n_unmasked_opaque_outside += (c_unmasked[3] == 255);
      n_wrong_masked += (c_masked[3] != 0);
      n_wrong_rgb += (c_rgb[3] != 0);
    }
  }
  CHECK(n_unmasked_opaque_outside > 0,
        "without the mask, the outside is shown with the color for 0 (the bug)");
  CHECK(n_wrong_masked == 0,
        "with the mask, the scalar filter makes the outside transparent and nothing else ("
          << n_wrong_masked << " pixels wrong)");
  CHECK(n_zero_color > 0, "zero voxels inside the image keep the opaque color for 0");
  CHECK(n_wrong_rgb == 0,
        "the RGB filter makes the outside transparent and nothing else (" << n_wrong_rgb
                                                                          << " pixels wrong)");

  // ---------------------------------------------------------------------------
  std::cout << "Back to orthogonal slicing" << std::endl;
  pipeline->SetUseOrthogonalSlicing(true);
  pipeline->Update();
  mask = pipeline->GetMaskOutput();
  n_not_full = 0;
  for (itk::ImageRegionConstIterator<MaskType> it(mask, mask->GetBufferedRegion()); !it.IsAtEnd(); ++it)
    n_not_full += (it.Get() != 255);
  CHECK(mask->GetBufferedRegion() == pipeline->GetOutput()->GetBufferedRegion() && n_not_full == 0,
        "the mask is 255 everywhere again");

  if (g_Failures)
  {
    std::cerr << g_Failures << " check(s) failed" << std::endl;
    return 1;
  }
  std::cout << "All checks passed" << std::endl;
  return 0;
}
