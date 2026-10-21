// Test: IRISSlicer extracts slices from a preview input whose buffered region
// is only the requested slice (as produced by the on-demand preprocessing
// pipelines in snake mode), as well as from a fully buffered main input.
//
// Regression test: the slicer compared the slice index against the size of
// the buffered region, so slices of a preview pipeline (buffered size 1 along
// the slice axis) were treated as out of range and came out as zeros. The
// speed image stayed black after training the classifier.

#include "IRISSlicer.h"
#include "itkImage.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkShiftScaleImageFilter.h"
#include <iostream>

using ImageType = itk::Image<float, 3>;
using SliceType = itk::Image<float, 2>;
using SlicerType = IRISSlicer<ImageType, SliceType, ImageType>;
using FilterType = itk::ShiftScaleImageFilter<ImageType, ImageType>;

static int n_failures = 0;

// A value that identifies each voxel
static float VoxelValue(const itk::Index<3> &idx)
{
  return 1.0f + idx[0] + 100.0f * idx[1] + 10000.0f * idx[2];
}

static ImageType::Pointer MakeImage(bool ramp)
{
  ImageType::RegionType region;
  region.SetSize({{7, 5, 4}});
  auto img = ImageType::New();
  img->SetRegions(region);
  img->Allocate();
  itk::ImageRegionIteratorWithIndex<ImageType> it(img, region);
  for(; !it.IsAtEnd(); ++it)
    it.Set(ramp ? VoxelValue(it.GetIndex()) : 0.0f);
  return img;
}

// Check that the slicer output is the slice of the ramp image along 'axis' at
// 'slice', or all zeros if 'expect_zero' is set
static void CheckSlice(SlicerType *slicer, unsigned int axis, int slice,
                       bool expect_zero, const char *what)
{
  unsigned int pix_axis = (axis == 0) ? 1 : 0;
  unsigned int line_axis = (axis == 2) ? 1 : 2;

  slicer->SetSliceDirectionImageAxis(axis);
  slicer->SetPixelDirectionImageAxis(pix_axis);
  slicer->SetLineDirectionImageAxis(line_axis);
  slicer->SetPixelTraverseForward(true);
  slicer->SetLineTraverseForward(true);
  slicer->SetSliceIndex(slice);

  // As in the renderer: the slice size changes with the slicing direction
  slicer->UpdateLargestPossibleRegion();

  SliceType *out = slicer->GetOutput();
  int n_bad = 0;
  itk::ImageRegionIteratorWithIndex<SliceType> it(out, out->GetBufferedRegion());
  for(; !it.IsAtEnd(); ++it)
    {
    itk::Index<3> idx;
    idx[axis] = slice;
    idx[pix_axis] = it.GetIndex()[0];
    idx[line_axis] = it.GetIndex()[1];
    float expected = expect_zero ? 0.0f : VoxelValue(idx);
    if(it.Get() != expected)
      n_bad++;
    }

  if(n_bad)
    {
    std::cerr << "FAIL: " << what << " axis " << axis << " slice " << slice
              << ": " << n_bad << " wrong pixels" << std::endl;
    n_failures++;
    }
}

int main(int, char *[])
{
  // 1. Main input only, fully buffered
    {
    auto slicer = SlicerType::New();
    auto ramp = MakeImage(true);
    slicer->SetInput(ramp);
    for(unsigned int axis = 0; axis < 3; axis++)
      {
      int n = ramp->GetBufferedRegion().GetSize(axis);
      for(int slice = 0; slice < n; slice++)
        CheckSlice(slicer, axis, slice, false, "main input");

      // Slices outside of the image are filled with zeros
      CheckSlice(slicer, axis, -1, true, "main input, outside");
      CheckSlice(slicer, axis, n, true, "main input, outside");
      }
    }

  // 2. Preview input from a pipeline, which only buffers the requested slice.
  // The main input is empty (as for the speed image before it is computed).
    {
    auto slicer = SlicerType::New();
    auto empty = MakeImage(false);
    auto ramp = MakeImage(true);
    auto filter = FilterType::New();
    filter->SetInput(ramp);
    slicer->SetInput(empty);
    slicer->SetPreviewInput(filter->GetOutput());

    // Make the preview newer than the main input, so that it is used
    filter->Modified();

    for(unsigned int axis = 0; axis < 3; axis++)
      {
      int n = ramp->GetBufferedRegion().GetSize(axis);
      for(int slice = 0; slice < n; slice++)
        {
        CheckSlice(slicer, axis, slice, false, "preview input");

        // The preview should only have computed the requested slice
        auto preview_region = filter->GetOutput()->GetBufferedRegion();
        if(preview_region.GetSize(axis) != 1 || preview_region.GetIndex(axis) != slice)
          {
          std::cerr << "FAIL: preview buffered region is not the requested slice: "
                    << preview_region << std::endl;
          n_failures++;
          }
        }
      }
    }

  if(n_failures)
    {
    std::cerr << n_failures << " check(s) failed" << std::endl;
    return 1;
    }

  std::cout << "All checks passed" << std::endl;
  return 0;
}
