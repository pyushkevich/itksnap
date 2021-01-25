#include <vnl_vector_fixed.h>
#include <vnl_vector_fixed.hxx>

#include <vnl_complex_traits.h>

template <> struct VNL_EXPORT vnl_complex_traits<bool>
{
  enum { isreal = true };
  static bool conjugate(bool x) { return x; }
  static std::complex<bool> complexify(bool x) { return std::complex<bool>(x, (bool)0); }
};

#include <vnl_c_vector.hxx>

// Some types that are not instantiated in VNL itself
VNL_VECTOR_FIXED_INSTANTIATE(bool, 2);
VNL_VECTOR_FIXED_INSTANTIATE(bool, 3);
VNL_VECTOR_FIXED_INSTANTIATE(bool, 4);

template class vnl_c_vector<bool>;

#include <vnl_vector.hxx>
VNL_VECTOR_INSTANTIATE(bool);

#include <vnl_vector_ref.hxx>
VNL_VECTOR_REF_INSTANTIATE(bool);

#include <vnl_matrix.hxx>
VNL_MATRIX_INSTANTIATE(bool);
