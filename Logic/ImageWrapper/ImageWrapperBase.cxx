#include "ImageWrapperBase.h"
#include "itkImageBase.h"
#include "IRISException.h"




vnl_matrix_fixed<double, 4, 4>
ImageWrapperBase
::ConstructNiftiSform(vnl_matrix<double> m_dir,
                      vnl_vector<double> v_origin,
                      vnl_vector<double> v_spacing)
{
  // Set the NIFTI/RAS transform
  vnl_matrix<double> m_ras_matrix;
  vnl_diag_matrix<double> m_scale, m_lps_to_ras;
  vnl_vector<double> v_ras_offset;

  // Compute the matrix
  m_scale.set(v_spacing);
  m_lps_to_ras.set(vnl_vector<double>(3, 1.0));
  m_lps_to_ras[0] = -1;
  m_lps_to_ras[1] = -1;
  m_ras_matrix = m_lps_to_ras * m_dir * m_scale;

  // Compute the vector
  v_ras_offset = m_lps_to_ras * v_origin;

  // Create the larger matrix
  vnl_vector<double> vcol(4, 1.0);
  vcol.update(v_ras_offset);

  vnl_matrix_fixed<double,4,4> m_sform;
  m_sform.set_identity();
  m_sform.update(m_ras_matrix);
  m_sform.set_column(3, vcol);
  return m_sform;
}

vnl_matrix_fixed<double, 4, 4>
ImageWrapperBase
::ConstructVTKtoNiftiTransform(vnl_matrix<double> m_dir,
                               vnl_vector<double> v_origin,
                               vnl_vector<double> v_spacing)
{
  vnl_matrix_fixed<double,4,4> vox2nii = ConstructNiftiSform(m_dir, v_origin, v_spacing);
  vnl_matrix_fixed<double,4,4> vtk2vox;
  vtk2vox.set_identity();
  for(size_t i = 0; i < 3; i++)
    {
    vtk2vox(i,i) = 1.0 / v_spacing[i];
    vtk2vox(i,3) = - v_origin[i] / v_spacing[i];
    }
  return vox2nii * vtk2vox;
}


ScalarRepresentationIterator
::ScalarRepresentationIterator(const VectorImageWrapperBase *wrapper)
  : m_Depth(NUMBER_OF_SCALAR_REPS, 1)
{
  assert(wrapper->GetNumberOfComponents() > 0);

  m_Depth[SCALAR_REP_COMPONENT] = wrapper->GetNumberOfComponents();
  m_Current = SCALAR_REP_COMPONENT;
  m_Index = 0;
}

ScalarRepresentationIterator &ScalarRepresentationIterator::operator ++()
{
  // Once at the end stay at the end
  if(m_Current == NUMBER_OF_SCALAR_REPS)
    return *this;

  // If there is room to grow in current rep, do it
  if(m_Index + 1 < m_Depth[(int) m_Current])
    {
    m_Index++;
    }
  else
    {
    m_Current++;
    m_Index = 0;
    }

  return *this;
}

bool ScalarRepresentationIterator::IsAtEnd() const
{
  return m_Current == NUMBER_OF_SCALAR_REPS;
}





