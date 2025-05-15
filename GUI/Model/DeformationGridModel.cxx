#include "DeformationGridModel.h"
#include "GenericImageData.h"
#include "GlobalUIModel.h"
#include "SNAPAppearanceSettings.h"
#include <itkImageLinearConstIteratorWithIndex.h>


DeformationGridModel
::DeformationGridModel()
{

}

int
DeformationGridModel::
GetVertices(ImageWrapperBase *layer, DeformationGridVertices &v) const
{
  typedef ImageWrapperBase::FloatVectorSliceType SliceType;

  // Draw the texture for the layer
  int nc = layer->GetNumberOfComponents();
  if (layer && (nc == 3 || (nc == 2 && m_Parent->GetSliceSize()[0] > 1 && m_Parent->GetSliceSize()[1] > 1 )))
    {
    // Create a pipeline that casts the slice to a floating point vector image
    // TODO: this involves new memory allocation in each call, in the future we might
    // want to create this pipeline when deformation grid visualization is enabled
    // and delete it when it is disabled
    SliceType *slice = layer->CreateCastToFloatVectorSlicePipeline(
          "DeformationGridModelCastToFloat",m_Parent->GetId());

    // Get the slice
    slice->GetSource()->UpdateLargestPossibleRegion();

    // The mapping between (index, phi[index]) and on-screen coordinate for a grid
    // point is linear (combines a bunch of transforms). To save time, we can
    // compute this mapping once at the beginning of the loop. We also know that the
    // index will only be going up by one at each iteration
    itk::Index<2> ind;
    Vector3d phi, G0, d_grid_d_phi[3], d_grid_d_ind[2];

    // Compute the initial displacement G0
    ind.Fill(0); phi.fill(0.0f);
    G0 = m_Parent->ComputeGridPosition(phi, ind, layer);

    // Compute derivative of grid displacement wrt warp components
    for(int a = 0; a < 3; a++)
      {
      ind.Fill(0); phi.fill(0.0f);
      phi[a] = 1.0f;
      d_grid_d_phi[a] = m_Parent->ComputeGridPosition(phi, ind, layer) - G0;
      }

    // Compute derivative of grid displacement wrt index components
    for(int b = 0; b < 2; b++)
      {
      ind.Fill(0); phi.fill(0.0f);
      ind[b] = 1;
      d_grid_d_ind[b] = m_Parent->ComputeGridPosition(phi, ind, layer) - G0;
      }

    size_t nd0[2] {0, 0}, nd1[2] {0, 0};
    bool counted = false;

    // Iterate line direction
    for(int d = 0; d < 2; d++)
      {
      // The current matrix is such that we should be drawing in pixel coordinates.
      typedef itk::ImageLinearConstIteratorWithIndex<SliceType> IterType;
      IterType it1(slice, slice->GetBufferedRegion());
      it1.SetDirection(d);
      it1.GoToBegin();

      int vox_increment;
      if(layer->IsSlicingOrthogonal())
        {
        // Figure out how frequently to sample lines. The spacing on the screen should be at
        // most every 4 pixels. Zoom is in units of px/mm. Spacing is in units of mm/vox, so
        // zoom * spacing is (display pixels) / (image voxels).
        double disp_pix_per_vox = m_Parent->GetSliceSpacing()[d] * m_Parent->GetViewZoom();
        vox_increment = (int) ceil(8.0 / disp_pix_per_vox);
        }
      else
        {
        // The slice is in screen pixel units already - so just 8!
        vox_increment = 8;
        }

      counted = false;
      while( !it1.IsAtEnd() )
        {
        // Do we draw this line?
        if(it1.GetIndex()[1-d] % vox_increment == 0)
          {
          ++nd0[d];

          // Set up the current position and increment
          Vector3d G1 = G0 +
              (d_grid_d_ind[0] * (double) (it1.GetIndex()[0])) +
              (d_grid_d_ind[1] * (double) (it1.GetIndex()[1]));

          while( !it1.IsAtEndOfLine() )
            {
            if (!counted)
              ++nd1[d];

            // Read the pixel
            auto pix = it1.Get();

            // Alternative version
            Vector3d xDispSlice = G1 +
                (d_grid_d_phi[0] * (double) (pix[0])) +
                (d_grid_d_phi[1] * (double) (pix[1])) +
                (d_grid_d_phi[2] * (double) (pix[2]));

            v.vvec.push_back(xDispSlice[0]);
            v.vvec.push_back(xDispSlice[1]);

            // Add the displacement
            ++it1;

            // Update the current position
            G1 += d_grid_d_ind[d];
            }
          counted = true;
          }

        it1.NextLine();
        }
      }

    v.nline[0] = nd0[0];
    v.nvert[0] = nd1[0];
    v.nline[1] = nd0[1];
    v.nvert[1] = nd1[1];

    // Release the mini-pipeline
    layer->ReleaseInternalPipeline("DeformationGridModelCastToFloat", m_Parent->GetId());
    }

  return EXIT_SUCCESS;
}
