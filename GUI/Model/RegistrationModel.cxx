#include "RegistrationModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "IRISImageData.h"
#include "GlobalUIModel.h"
#include "SystemInterface.h"
#include "HistoryManager.h"
#include "itkQuaternionRigidTransform.h"
#include "itkTransformFileWriter.h"
#include "itkTransformFileReader.h"
#include "itkTransformFactory.h"
#include "AffineTransformHelper.h"
#include "ImageFunctions.h"
#include "itkEuler3DTransform.h"
#include "vnl/algo/vnl_svd.h"

#include "OptimizationProgressRenderer.h"


const unsigned long RegistrationModel::NOID = (unsigned long)(-1);

RegistrationModel::RegistrationModel()
{
  m_MovingLayerModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetMovingLayerValueAndRange,
        &Self::SetMovingLayerValue);

  m_InteractiveToolModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetInteractiveToolValue,
        &Self::SetInteractiveToolValue);

  m_EulerAnglesModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetEulerAnglesValueAndRange,
        &Self::SetEulerAnglesValue);

  m_TranslationModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetTranslationValueAndRange,
        &Self::SetTranslationValue);

  m_ScalingModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetScalingValueAndRange,
        &Self::SetScalingValue);

  m_FlipModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetFlipValue,
        &Self::SetFlipValue);

  m_LogScalingModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetLogScalingValueAndRange,
        &Self::SetLogScalingValue);

  m_RotationCenter = Vector3ui(0, 0, 0);

  // Set up the automatic registration parameters

  // Registration mode
  // TODO: add the other modes
  TransformationDomain transform_domain;
  transform_domain[RIGID] = "Rigid";
  transform_domain[AFFINE] = "Affine";
  m_TransformationModel = NewConcreteProperty(RIGID, transform_domain);

  // Registration metric
  SimilarityMetricDomain metric_domain;
  metric_domain[NMI] = "Mutual information";
  metric_domain[NCC] = "Cross-correlation";
  metric_domain[SSD] = "Intensity difference";
  m_SimilarityMetricModel = NewConcreteProperty(NMI, metric_domain);

  // Mask model
  m_UseSegmentationAsMaskModel = NewSimpleConcreteProperty(false);

  m_LastMetricValueModel = NewSimpleConcreteProperty(0.0);

  // Multi-resolution
  m_CoarsestResolutionLevelModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCoarsestResolutionLevelValueAndRange,
        &Self::SetCoarsestResolutionLevelValue);

  m_FinestResolutionLevelModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetFinestResolutionLevelValueAndRange,
        &Self::SetFinestResolutionLevelValue);

  // Initialize the moving layer ID to be -1
  m_MovingLayerId = NOID;

  // Set up the metric renderer
  m_RegistrationProgressRenderer = OptimizationProgressRenderer::New();
  m_RegistrationProgressRenderer->SetModel(this);

  m_Driver = NULL;
  m_Parent = NULL;
  m_GreedyAPI = NULL;
}

RegistrationModel::~RegistrationModel()
{

}


void RegistrationModel::ResetOnMainImageChange()
{
  if(m_Driver->GetIRISImageData()->IsMainLoaded())
    {
    ImageWrapperBase* main_img = m_Driver->GetIRISImageData()->GetMain();
    Vector3ui main_dim = main_img->GetSize();

    // Reset the center of rotation
    Vector3ui center;
    for(int i = 0; i < 3; i++)
      center[i] = main_dim[i] / 2;
    this->SetRotationCenter(center);

    // Reset the multi-resolution pyramid based on some heuristics
    int dim_min = main_dim.min_value();
    int dim_max = main_dim.max_value();

    // The coarsest factor may not exceed smallest image dimension
    int coarse_ub_1 = (int) (log2(dim_min));

    // It does not make sense to do multi-resolution after the largest
    // dimension has been reduced below 32. For example, for an image
    // 300x140x120, 8x gives 37x17x15 but 16x would be pointless. But
    // for an image that's 512x512x200 we want to offer 16x
    int coarse_ub_2 = (int) (log2(dim_max / 32));

    // We typically want to do registration at the at most two coarsest levels
    m_CoarsestResolutionLevel = std::max(0, std::min(coarse_ub_1, coarse_ub_2));
    m_FinestResolutionLevel = std::max(0, m_CoarsestResolutionLevel - 1);

    // Update the domain with "1x", "2x", and so on
    m_ResolutionLevelDomain.clear();
    for(int i = 0; i <= m_CoarsestResolutionLevel; i++)
      {
      std::ostringstream oss;
      oss << (1 << i) << "x";
      m_ResolutionLevelDomain[i] = oss.str();
      }
    }
}

/*
class RegistrationModelCostFunction : public vnl_least_squares_function
{
public:
  typedef RegistrationModel::Vec3 Vec3;
  typedef RegistrationModel::Mat3 Mat3;
  typedef RegistrationModel::Mat4 Mat4;

  RegistrationModelCostFunction(const RegistrationModel *model, const Mat4 &target)
    : vnl_least_squares_function(12, 12, vnl_least_squares_function::no_gradient)
  {
    m_Model = model;
    m_Target = target;
    m_Iter = 0;
  }

  virtual void f(vnl_vector<double> const& x, vnl_vector<double>& fx) ITK_OVERRIDE
  {
    vnl_vector_fixed<double, 3> euler_angles = x.extract(3, 0);
    vnl_vector_fixed<double, 3> translation = x.extract(3, 3);
    vnl_vector_fixed<double, 3> scales = x.extract(3, 6);
    vnl_vector_fixed<double, 3> shear_euler_angles = x.extract(3, 9);

    // Get the matrix
    Mat4 M = m_Model->MapParametersToAffineTransform(
               euler_angles, translation, scales, shear_euler_angles);

    // Get the target affine matrix
    double del = (M - m_Target).frobenius_norm();

    // Fill out the residuals
    for(unsigned int i = 0; i < 3; i++)
      for(unsigned int j = 0; j < 4; j++)
        fx[i * 4 + j] = m_Target(i,j) - M(i,j);

    // Report
    printf("Powell: Iter %5d   Cost %8.6f   Param: ", m_Iter++, del);
    std::cout << x << std::endl;
  }

protected:
  const RegistrationModel *m_Model;
  unsigned int m_Iter;
  Mat4 m_Target;
};
*/

#include <vnl_symmetric_eigensystem.h>

void RegistrationModel::UpdateManualParametersFromWrapper(bool reset_flips, bool force_update)
{
  // If there is no layer, we just invalidate the parameters
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  if(!layer)
    {
    // Invalidate the parameters
    m_ManualParam.LayerID = NOID;
    return;
    }

  // Check if the data we have is already current
  if(!force_update &&
     m_MovingLayerId == m_ManualParam.LayerID &&
     layer->GetITKTransform()->GetTimeStamp() <= m_ManualParam.UpdateTime)
    return;

  // Get the current transform
  this->GetMovingTransform(m_ManualParam.AffineMatrix, m_ManualParam.AffineOffset);

  // Perform the SVD of the affine matrix, which allows us to do polar decomposition
  Mat3 A = m_ManualParam.AffineMatrix.GetVnlMatrix();
  Vec3 b = m_ManualParam.AffineOffset.GetVnlVector();
  vnl_svd<double> svd(A);

  // Get the orthonormal part of A and the positive semi-definite part of A
  Mat3 R = svd.U() * svd.V().transpose();
  Mat3 P = svd.U() * svd.W() * svd.U().transpose();
  Mat3 Pinv = svd.U() * svd.Winverse() * svd.U().transpose();

  // If R matrix has negative determinant, multiplyyboth matrices by -1, making it
  // a proper rotation matrix
  double det_R = vnl_det(R);
  if(det_R < 0.0)
    {
    R *= -1.0;
    P *= -1.0;
    Pinv *= -1.0;
    }

  // Get the direction matrix (from voxel coordinates to physical coordinates)
  Mat3 Q = this->GetMovingLayerWrapper()->GetImageBase()->GetDirection().GetVnlMatrix();

  // Get the inverse direction matrix
  Mat3 Qinv = this->GetMovingLayerWrapper()->GetImageBase()->GetDirection().GetInverse();

  // The matrix P must be mapped into the voxel space of the moving image, this is where the
  // scaling and flipping actually makes sense
  Mat3 Pvox = Qinv * P * Q;

  // Now the Pvox matrix must be decomposed into scaling and shearing parts. This can be done
  // using eigendecomposition, but we must be careful to assign scaling and shearing to the
  // correct axes.
  vnl_symmetric_eigensystem<double> eigen(Pvox);
  Mat3 S; S.fill(0.0); S.set_diagonal(eigen.D.get_diagonal());
  Mat3 B = eigen.V;

  // The eigenvalues and vectors are sorted from largest to smallest, but we want them to be
  // in the order of x, y and z. Eigenvectors are the columns of B. Find the reorder matrix
  // which has the highest trace when multiplied with B, and this should be the reorder
  // matrix to use.
  Mat3 reorder[6], best_reorder;
  double max_reorder_fit;
  for(unsigned int a = 0, i = 0; a < 3; a++)
    {
    for(unsigned int b = 1; b <= 2; b++, i++)
      {
      // Create the reorder matrix
      reorder[i].fill(0.0);
      reorder[i](0,a) = 1.0;
      reorder[i](1,(a+b) % 3) = 1.0;
      reorder[i](2,(a+2*b) % 3) = 1.0;

      // Compute the dot products with the eigenvectors
      double fit = (reorder[i] * B).get_diagonal().one_norm();
      if(i == 0 || fit > max_reorder_fit)
        {
        max_reorder_fit = fit;
        best_reorder = reorder[i];
        }
      }
    }

  // Apply the reorder matrix to everything (check transpose here)
  B = B * best_reorder;
  S = best_reorder.transpose() * S * best_reorder;

  // If B has negative determinant, multiply it by -1. This has no effect on S
  if(vnl_det(B) < 0.0)
    B *= -1.0;

  // Finally, at this point we have matrices of scales, shears and rotation. We have not
  // yet handled the flips, but we can do that later I guess. Now we have to work out all
  // the translations. We know that point x in physical space moves to Ax + b. But now we
  // have to set all the translations. There is some ugly arithmetic to compute the
  // translation vector, taking into account that the rotation is around the center of
  // rotation and the P matrix is around the center of the moving image

  // Get the center of rotation in LPS coords
  itk::Point<double, 3> ptCenter;
  layer->GetReferenceSpace()->TransformIndexToPhysicalPoint(
        to_itkIndex(m_RotationCenter), ptCenter);
  Vec3 crot = ptCenter.GetVnlVector();

  // Get the moving image center in LPS coords
  itk::ContinuousIndex<double,3> cimg_idx;
  itk::Point<double, 3> cimg_pt;
  for(unsigned int i = 0; i < 3; i++)
    cimg_idx[i] =
        layer->GetImageBase()->GetLargestPossibleRegion().GetIndex()[i] +
        0.5 * layer->GetImageBase()->GetLargestPossibleRegion().GetSize()[i];
  layer->GetImageBase()->TransformContinuousIndexToPhysicalPoint(cimg_idx, cimg_pt);

  // This is the center of the moving image in physical space.
  Vec3 cimg = cimg_pt.GetVnlVector();

  // Compute the translation vector
  Vec3 v_tran =  Pinv * (b - cimg) + cimg + R * crot - crot;

  // We now have two rotation matrices that must be mapped to euler angles.
  m_ManualParam.EulerAngles = this->MapRotationMatrixToEulerAngles(R);
  m_ManualParam.Translation = v_tran;
  m_ManualParam.ShearingEulerAngles = this->MapRotationMatrixToEulerAngles(B);

  // Handle the scaling and the flips
  for(unsigned int a = 0; a < 3; a++)
    {
    m_ManualParam.Flip[a] = S(a,a) < 0.0;
    m_ManualParam.Scaling[a] = fabs(S(a,a));
    }

  // Sanity check: compare A/b to what we would get from the parameters
  Mat4 M_test = this->MapParametersToAffineTransform(m_ManualParam.EulerAngles,
                                                     m_ManualParam.Translation,
                                                     m_ManualParam.Scaling,
                                                     m_ManualParam.ShearingEulerAngles);


  Mat4 M_input = this->make_homog(A, b);
  double error_M = (M_test - M_input).frobenius_norm();
  if(error_M > 1.0e-6)
    {
    std::cerr << "Matrix/Parameter mismatch" << std::endl;
    std::cerr << "Input Affine Transform:" << std::endl;
    std::cerr << M_input << std::endl;
    std::cerr << "Reconstructed Affine Transform:" << std::endl;
    std::cerr << M_test << std::endl;
    std::cerr << "P-test:" << (P * Pinv) << std::endl;
    std::cerr << "Done" << std::endl;
    }

  // Compute the range of the translation. This is based on the bounding box
  // that contains both images in their native space
  Vector3d ext_min_ref, ext_max_ref, ext_min_mov, ext_max_mov;
  GetImagePhysicalExtents(layer->GetImageBase(), ext_min_mov, ext_max_mov);
  GetImagePhysicalExtents(layer->GetReferenceSpace(), ext_min_ref, ext_max_ref);
  Vector3d ext_min = vector_min(ext_min_ref, ext_min_mov);
  Vector3d ext_max = vector_max(ext_max_ref, ext_max_mov);
  Vector3d ext_range = ext_max - ext_min;

  // Compute the step size of the translation - it should be smaller than 1/10 of the min-spacing
  double min_spacing = 1e100;
  for(int i = 0; i < 3; i++)
    {
    min_spacing = std::min(layer->GetImageBase()->GetSpacing()[i], min_spacing);
    min_spacing = std::min(layer->GetReferenceSpace()->GetSpacing()[i], min_spacing);
    }

  double tran_step = pow(10, floor(log10(min_spacing / 10)));

  m_ManualParam.TranslationRange.Set(-ext_range, ext_range, Vector3d(tran_step, tran_step, tran_step));
  m_ManualParam.LayerID = m_MovingLayerId;
  m_ManualParam.UpdateTime = layer->GetITKTransform()->GetTimeStamp();

  this->InvokeEvent(ModelUpdateEvent());
}

RegistrationModel::Mat4
RegistrationModel::make_homog(const Mat3 &A, const Vec3 &b)
{
  Mat4 M; M.set_identity();
  for(unsigned int i = 0; i < 3; i++)
    {
    for(unsigned int j = 0; j < 3; j++)
      M(i,j) = A(i,j);
    M(i,3) = b[i];
    }
  return M;
}

RegistrationModel::Mat3
RegistrationModel::MapEulerAnglesToRotationMatrix(const Vec3 &euler_angles) const
{
  // Create a 4x4 transform that rotates by Euler angles while preserving the center
  typedef itk::Euler3DTransform<double> EulerTransform;
  EulerTransform::Pointer euler = EulerTransform::New();
  euler->SetRotation(euler_angles[0], euler_angles[1], euler_angles[2]);
  return euler->GetMatrix().GetVnlMatrix();
}

RegistrationModel::Vec3
RegistrationModel::MapRotationMatrixToEulerAngles(const Mat3 &rotation) const
{
  typedef itk::Euler3DTransform<double> EulerTransform;
  EulerTransform::Pointer euler = EulerTransform::New();
  try
    {
    euler->SetMatrix(ITKMatrixType(rotation));
    Vec3 ea(euler->GetAngleX(), euler->GetAngleY(), euler->GetAngleZ());

    // Make sure the inverse is right
    Mat3 Rtest = this->MapEulerAnglesToRotationMatrix(ea);
    if((Rtest-rotation).frobenius_norm() > 1e-6)
      {
      std::cerr << "R mistmatch: " << rotation << Rtest << std::endl;
      std::cerr << "rotation det: " << vnl_det(rotation) << ", " << vnl_det(Rtest) << std::endl;
      }
    return ea;
    }
  catch (itk::ExceptionObject &)
    {
    std::cerr << "Non-orthogonal matrix" << std::endl;
    return Vec3(0.0, 0.0, 0.0);
    }
}

RegistrationModel::Mat4
RegistrationModel::MapParametersToAffineTransform(
    const Vec3 &euler_angles, const Vec3 &translation,
    const Vec3 &scales, const Vec3 &shear_euler_angles) const
{
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();

  // Convert the first set of Euler angles to a matrix
  Mat3 R = MapEulerAnglesToRotationMatrix(euler_angles);

  // Compute the offset to that the rotation preserves the center
  itk::Point<double, 3> ptCenter;
  layer->GetReferenceSpace()->TransformIndexToPhysicalPoint(
        to_itkIndex(m_RotationCenter), ptCenter);
  Vec3 crot = ptCenter.GetVnlVector();

  // Compute the rotation around the center
  Mat4 M_rotation = make_homog(R, crot - R * crot);

  // Create a 4x4 transform that handles translation
  Mat3 eye3; eye3.set_identity();
  Mat4 M_translation = make_homog(eye3, translation);

  // Create the rigid transform in physical space.
  Mat4 M_rigid = M_translation * M_rotation;

  // The scaling, shearing and flipping parameters are specified relative to the moving image
  // voxel coordinate frame (that's what makes sense, otherwise as you rotate the image, the
  // scaling will turn into shearing. Also these transformations keep the center of the moving
  // image intact, in other words, the scaling and shearing is relative to the moving image center
  Mat3 S; S.set_identity();
  for(unsigned int i = 0; i < 3; i++)
    S(i,i) = scales[i] * (m_ManualParam.Flip[i] ? -1.0 : 1.0);

  // Create a shearing matrix
  Mat3 B = this->MapEulerAnglesToRotationMatrix(shear_euler_angles);

  // The shearing matrix
  Mat3 BtSB = B * S * B.transpose();

  // Get the direction matrix (from voxel coordinates to physical coordinates)
  Mat3 Q = this->GetMovingLayerWrapper()->GetImageBase()->GetDirection().GetVnlMatrix();

  // Get the inverse direction matrix
  Mat3 Qinv = this->GetMovingLayerWrapper()->GetImageBase()->GetDirection().GetInverse();

  // Get the moving image center in LPS coords
  itk::ContinuousIndex<double,3> cimg_idx;
  itk::Point<double, 3> cimg_pt;
  for(unsigned int i = 0; i < 3; i++)
    cimg_idx[i] =
        layer->GetImageBase()->GetLargestPossibleRegion().GetIndex()[i] +
        0.5 * layer->GetImageBase()->GetLargestPossibleRegion().GetSize()[i];
  layer->GetImageBase()->TransformContinuousIndexToPhysicalPoint(cimg_idx, cimg_pt);

  // This is the center of the moving image in physical space.
  Vec3 cimg = cimg_pt.GetVnlVector();

  // Get the matrix in physical space that corresponds to shearing/scaling in voxel space
  Mat3 S_scale_shear = Q * BtSB * Qinv;

  // Shearing scaling transformation should keep the center of the moving image in the same place
  Mat4 M_scale_shear = make_homog(S_scale_shear, cimg - S_scale_shear * cimg);

  // This is the trasnform in RAS space
  Mat4 M = M_scale_shear * M_rigid;

  // Return this matrix
  return M;
}


void RegistrationModel::UpdateWrapperFromManualParameters()
{  
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  assert(layer);

  // Get the rotation center in world coordinates
  Mat4 M = MapParametersToAffineTransform(
             m_ManualParam.EulerAngles,
             m_ManualParam.Translation,
             m_ManualParam.Scaling,
             m_ManualParam.ShearingEulerAngles);

  // Store the affine transform
  m_ManualParam.AffineMatrix.GetVnlMatrix() = M.extract(3,3);
  m_ManualParam.AffineOffset.SetVnlVector(M.get_column(3).extract(3));

  // Create a new transform
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> AffineTransform;
  AffineTransform::Pointer affine = AffineTransform::New();
  affine->SetMatrix(m_ManualParam.AffineMatrix);
  affine->SetOffset(m_ManualParam.AffineOffset);

  // Update the layer's transform
  layer->SetITKTransform(layer->GetReferenceSpace(), affine);

  // Update the state of the cache
  m_ManualParam.LayerID = m_MovingLayerId;
  m_ManualParam.UpdateTime = layer->GetITKTransform()->GetTimeStamp();
}

void RegistrationModel::ApplyRotation(const Vector3d &axis, double theta)
{
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  assert(layer);

  // Create a quaternion transform that will be applied on top of the existing
  // transform
  typedef itk::QuaternionRigidTransform<double> QuaterionTransform;
  QuaterionTransform::Pointer rotation = QuaterionTransform::New();
  QuaterionTransform::VnlQuaternionType quat(axis, theta);

  // Get the rotation center in world coordinates
  itk::Point<double, 3> ptCenter;
  layer->GetReferenceSpace()->TransformIndexToPhysicalPoint(
        to_itkIndex(m_RotationCenter), ptCenter);

  rotation->SetRotation(quat);

  // Figure out the center of rotation. The center of rotation should be preserved by
  // the rotation, i.e. A * ptCenter + b = R(A * ptCenter + b) + \beta
  vnl_matrix_fixed<double, 3, 3> A = m_ManualParam.AffineMatrix.GetVnlMatrix();
  vnl_matrix_fixed<double, 3, 3> R = rotation->GetMatrix().GetVnlMatrix();
  vnl_vector_fixed<double, 3> b = m_ManualParam.AffineOffset.GetVnlVector();
  vnl_vector_fixed<double, 3> C = ptCenter.GetVnlVector();

  m_ManualParam.AffineMatrix = A * R;
  m_ManualParam.AffineOffset.SetVnlVector(A * C + b - A * (R * C));

  // Update the transform
  this->SetMovingTransform(
        m_ManualParam.AffineMatrix,
        m_ManualParam.AffineOffset);
}

void RegistrationModel::ApplyTranslation(const Vector3d &tran)
{
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  assert(layer);

  // Get the current transform
  ITKMatrixType matrix; ITKVectorType offset;
  this->GetMovingTransform(matrix, offset);

  // Add the translation to the offset
  offset.SetVnlVector(offset.GetVnlVector() - matrix.GetVnlMatrix() * tran);

  // Update the offset
  this->SetMovingTransform(matrix, offset);
}



void RegistrationModel::SetRotationCenter(const Vector3ui &pos)
{
  m_RotationCenter = pos;
  this->UpdateManualParametersFromWrapper(false, true);
}

void RegistrationModel::SetMovingTransform(const RegistrationModel::ITKMatrixType &matrix, const RegistrationModel::ITKVectorType &offset)
{
  // Create a new transform
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> AffineTransform;
  AffineTransform::Pointer affine = AffineTransform::New();

  // Set the matrix of the new transform y = R ( A x + b ) + z
  affine->SetMatrix(matrix);
  affine->SetOffset(offset);

  // Create a new euler transform
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  layer->SetITKTransform(layer->GetReferenceSpace(), affine);

  // Update our parameters
  this->UpdateManualParametersFromWrapper(false, false);
}

void RegistrationModel::GetMovingTransform(ITKMatrixType &matrix, ITKVectorType &offset)
{
  // Get the transform
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  const ImageWrapperBase::ITKTransformType *transform = layer->GetITKTransform();

  // TODO: in the future it might make more sense to stick to a single kind of
  // transform in the ImageWrapper instead of allowing different transform
  // classes. Using multiple classes seems pointless.
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformBase;
  const TransformBase *tb = dynamic_cast<const TransformBase *>(transform);
  matrix.SetIdentity();
  offset.Fill(0.0);
  if(tb)
    {
    matrix = tb->GetMatrix();
    offset = tb->GetOffset();
    }
}

ImageWrapperBase *RegistrationModel::GetMovingLayerWrapper() const
{
  if(m_MovingLayerId == NOID)
    return NULL;

  return m_Driver->GetCurrentImageData()->FindLayer(m_MovingLayerId, false, OVERLAY_ROLE);
}

void RegistrationModel::SetIterationCommand(itk::Command *command)
{
  m_IterationCommand = command;
}

#include "GreedyAPI.h"
void RegistrationModel::RunAutoRegistration()
{
  // Obtain the fixed and moving images.
  ImageWrapperBase *fixed = this->GetParent()->GetDriver()->GetCurrentImageData()->GetMain();
  ImageWrapperBase *moving = this->GetMovingLayerWrapper();

  // TODO: for now, we are not supporting vector image registration, only registration between
  // scalar components; and we use the default scalar component.
  SmartPtr<ScalarImageWrapperBase::FloatVectorImageSource> castFixed =
      fixed->GetDefaultScalarRepresentation()->CreateCastToFloatVectorPipeline();
  castFixed->UpdateOutputInformation();

  SmartPtr<ScalarImageWrapperBase::FloatVectorImageSource> castMoving =
      moving->GetDefaultScalarRepresentation()->CreateCastToFloatVectorPipeline();
  castMoving->UpdateOutputInformation();

  // Caster for the mask image - declared here so that SmartPtr does not go out of scope
  SmartPtr<ScalarImageWrapperBase::FloatImageSource> castMask;

  // Set up the parameters for greedy registration
  GreedyParameters param;
  GreedyParameters::SetToDefaults(param);

  // Create an API object
  m_GreedyAPI = new GreedyAPI();

  // Configure the fixed and moving images
  ImagePairSpec ip;
  ip.weight = 1.0;
  ip.fixed = "FIXED_IMAGE";
  ip.moving = "MOVING_IMAGE";
  param.inputs.push_back(ip);

  // Pass the actual images to the cache
  m_GreedyAPI->AddCachedInputObject(ip.fixed, castFixed->GetOutput());
  m_GreedyAPI->AddCachedInputObject(ip.moving, castMoving->GetOutput());

  // Mask image
  if(this->GetUseSegmentationAsMask())
    {
    param.gradient_mask = "GRADIENT_MASK";
    ImageWrapperBase *seg = this->GetParent()->GetDriver()->GetSelectedSegmentationLayer();
    castMask = seg->GetDefaultScalarRepresentation()->CreateCastToFloatPipeline();
    castMask->UpdateLargestPossibleRegion();
    m_GreedyAPI->AddCachedInputObject(param.gradient_mask, castMask->GetOutput());
    }

  // Set up the metric
  switch(m_SimilarityMetricModel->GetValue())
    {
    case NCC:
      param.metric = GreedyParameters::NCC;
      param.metric_radius = std::vector<int>(3, 4);
      break;
    case NMI:
      param.metric = GreedyParameters::NMI;
      break;
    default:
      param.metric = GreedyParameters::SSD;
      break;
    };

  // Set up the degrees of freedom
  if(m_TransformationModel->GetValue() == RIGID)
    param.affine_dof = GreedyParameters::DOF_RIGID;
  else
    param.affine_dof = GreedyParameters::DOF_AFFINE;

  // Set up the pyramid
  param.iter_per_level.clear();
  for(int k = m_CoarsestResolutionLevel; k >= 0; k--)
    {
    if(k >= m_FinestResolutionLevel)
      param.iter_per_level.push_back(100);
    else
      param.iter_per_level.push_back(0);
    }

  // Create a transform spec
  param.affine_init_mode = RAS_FILENAME;
  param.affine_init_transform.filename = "INPUT_TRANSFORM";
  param.affine_init_transform.exponent = 1;

  // Pass the input transformation object to the cache
  ITKMatrixType matrix; ITKVectorType offset;
  this->GetMovingTransform(matrix, offset);

  // Unfortunately, we have to cast to float, argh!
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformType;
  TransformType::Pointer tran = TransformType::New();
  tran->SetMatrix(matrix);
  tran->SetOffset(offset);

  // Finally pass the float transform to the API
  m_GreedyAPI->AddCachedInputObject(param.affine_init_transform.filename, tran);

  // Pass the output string - same as the input transform
  param.output = param.affine_init_transform.filename;

  // Handle intermediate data
  if(m_IterationCommand)
    {
    typedef itk::MemberCommand<Self> CommandType;
    CommandType::Pointer cmd = CommandType::New();
    cmd->SetCallbackFunction(this, &RegistrationModel::IterationCallback);
    param.output_intermediate = param.affine_init_transform.filename;
    tran->AddObserver(itk::ModifiedEvent(), cmd);
    }

  // Run the registration
  m_GreedyAPI->RunAffine(param);

  // Now, the transform tran should hold our matrix and offset
  this->SetMovingTransform(tran->GetMatrix(), tran->GetOffset());

  // Delete the API
  delete(m_GreedyAPI); m_GreedyAPI = NULL;
}

void RegistrationModel::MatchByMoments(int order)
{
  // Obtain the fixed and moving images.
  ImageWrapperBase *fixed = this->GetParent()->GetDriver()->GetCurrentImageData()->GetMain();
  ImageWrapperBase *moving = this->GetMovingLayerWrapper();

  // TODO: for now, we are not supporting vector image registration, only registration between
  // scalar components; and we use the default scalar component.
  SmartPtr<ScalarImageWrapperBase::FloatVectorImageSource> castFixed =
      fixed->GetDefaultScalarRepresentation()->CreateCastToFloatVectorPipeline();
  castFixed->UpdateOutputInformation();

  SmartPtr<ScalarImageWrapperBase::FloatVectorImageSource> castMoving =
      moving->GetDefaultScalarRepresentation()->CreateCastToFloatVectorPipeline();
  castMoving->UpdateOutputInformation();

  // Set up the parameters for greedy registration
  GreedyParameters param;
  GreedyParameters::SetToDefaults(param);

  // Create an API object
  m_GreedyAPI = new GreedyAPI();

  // Configure the fixed and moving images
  ImagePairSpec ip;
  ip.weight = 1.0;
  ip.fixed = "FIXED_IMAGE";
  ip.moving = "MOVING_IMAGE";
  param.inputs.push_back(ip);

  // Pass the actual images to the cache
  m_GreedyAPI->AddCachedInputObject(ip.fixed, castFixed->GetOutput());
  m_GreedyAPI->AddCachedInputObject(ip.moving, castMoving->GetOutput());

  // Set up the metric
  switch(m_SimilarityMetricModel->GetValue())
    {
    case NCC:
      param.metric = GreedyParameters::NCC;
      param.metric_radius = std::vector<int>(3, 4);
      break;
    case NMI:
      param.metric = GreedyParameters::NMI;
      break;
    default:
      param.metric = GreedyParameters::SSD;
      break;
    };

  // Create a transform spec
  param.affine_init_mode = RAS_FILENAME;
  param.affine_init_transform.filename = "INPUT_TRANSFORM";
  param.affine_init_transform.exponent = 1;

  // Pass the input transformation object to the cache
  ITKMatrixType matrix; ITKVectorType offset;
  this->GetMovingTransform(matrix, offset);

  // Unfortunately, we have to cast to float, argh!
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformType;
  TransformType::Pointer tran = TransformType::New();
  tran->SetMatrix(matrix);
  tran->SetOffset(offset);

  // Finally pass the float transform to the API
  m_GreedyAPI->AddCachedInputObject(param.affine_init_transform.filename, tran);

  // Pass the output string - same as the input transform
  param.output = param.affine_init_transform.filename;

  // Set the order of the moments match
  param.moments_order = order;

  // Run the registration
  m_GreedyAPI->RunAlignMoments(param);

  // Now, the transform tran should hold our matrix and offset
  this->SetMovingTransform(tran->GetMatrix(), tran->GetOffset());

  // Delete the API
  delete(m_GreedyAPI); m_GreedyAPI = NULL;
}


void RegistrationModel::LoadTransform(const char *filename, TransformFormat format)
{
  // Read the transform
  SmartPtr<AffineTransformHelper::ITKTransformMOTB> tran;
  if(format == FORMAT_C3D)
    tran = AffineTransformHelper::ReadAsRASMatrix(filename);
  else
    tran = AffineTransformHelper::ReadAsITKTransform(filename);

  // Update the history
  this->GetParent()->GetSystemInterface()
      ->GetHistoryManager()->UpdateHistory("AffineTransform", filename, true);

  // Now, the transform tran should hold our matrix and offset
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  layer->SetITKTransform(layer->GetReferenceSpace(), tran);

  // Update our parameters
  this->UpdateManualParametersFromWrapper(true, false);
}


void RegistrationModel::SaveTransform(const char *filename, TransformFormat format)
{
  // Save based on the transform type
  if(format == FORMAT_C3D)
    {
    AffineTransformHelper::WriteAsRASMatrix(
          this->GetMovingLayerWrapper()->GetITKTransform(), filename);
    }
  else // (format == FORMAT_ITK)
    {
    AffineTransformHelper::WriteAsITKTransform(
          this->GetMovingLayerWrapper()->GetITKTransform(), filename);
    }

  // Update the history
  this->GetParent()->GetSystemInterface()
      ->GetHistoryManager()->UpdateHistory("AffineTransform", filename, true);
}

const RegistrationModel::MetricLog &
RegistrationModel::GetRegistrationMetricLog() const
{
  // Get the complete metric report
  return m_GreedyAPI->GetMetricLog();
}

void RegistrationModel::OnDialogClosed()
{
  // Don't leave the interactive mode on
  if(m_InteractiveToolModel->GetValue())
    m_InteractiveToolModel->SetValue(false);
}

void RegistrationModel
::ResliceMovingImage(InterpolationMethod method)
{
  ImageWrapperBase *moving = this->GetMovingLayerWrapper();
  assert(moving);

  // We can take advantage of existing facilities for interpolation and reslicing
  // in the ImageWrapper class
  SNAPSegmentationROISettings roi;
  roi.SetInterpolationMethod(method);
  roi.SetROI(m_Driver->GetCurrentImageData()->GetMain()->GetBufferedRegion());
  SmartPtr<ImageWrapperBase> reslice =
      moving->ExtractROI(roi, m_Parent->GetProgressCommand());

  // Give it a nickname
  reslice->SetCustomNickname(std::string("resliced ") + moving->GetNickname());

  // This wrapper can now be added to the main list of wrappers
  m_Driver->AddDerivedOverlayImage(moving, reslice, true);
}

bool RegistrationModel::CheckState(RegistrationModel::UIState state)
{
  switch(state)
    {
    case UIF_MOVING_SELECTION_AVAILABLE:
      return m_Driver->GetIRISImageData()->GetNumberOfLayers(OVERLAY_ROLE) > 0;
    case UIF_MOVING_SELECTED:
      return m_MovingLayerId != NOID;
    default:
      return false;
    }
}

void RegistrationModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;
  m_Driver = model->GetDriver();

  // Layer changes require a lot of things to be reset
  Rebroadcast(m_Driver, LayerChangeEvent(), ModelUpdateEvent());
  Rebroadcast(m_Driver, LayerChangeEvent(), StateMachineChangeEvent());

  // Changes to layers also affect state
  Rebroadcast(m_Driver, WrapperChangeEvent(), ModelUpdateEvent());

  // Interactive mode tool model is just a wrapper around the toolbar mode
  m_InteractiveToolModel->RebroadcastFromSourceProperty(
        m_Driver->GetGlobalState()->GetToolbarModeModel());
}

void RegistrationModel::OnUpdate()
{
  bool main_changed = this->m_EventBucket->HasEvent(MainImageDimensionsChangeEvent());
  bool layers_changed = this->m_EventBucket->HasEvent(LayerChangeEvent());
  bool wrapper_updated = this->m_EventBucket->HasEvent(WrapperChangeEvent());

  // Check for changes in the active layers
  if(layers_changed)
    {
    // Check if the active layer is still available
    if(!m_Driver->GetCurrentImageData()->FindLayer(m_MovingLayerId, false, OVERLAY_ROLE))
      {
      // Set the moving layer ID to the first available overlay
      LayerIterator it = m_Driver->GetCurrentImageData()->GetLayers(OVERLAY_ROLE);
      m_MovingLayerId = it.IsAtEnd() ? NOID : it.GetLayer()->GetUniqueId();
      }
    }

  // Specifically for when the main image changes
  if(main_changed)
    {
    // Reset the center of rotation to the image center
    this->ResetOnMainImageChange();
    }

  // If there is a wrapper update, update the transform parameters
  if(wrapper_updated || layers_changed)
    {
    // This will update the cached parameters
    this->UpdateManualParametersFromWrapper(true, false);
    }
}

void RegistrationModel::SetCenterOfRotationToCursor()
{
  this->SetRotationCenter(m_Driver->GetCursorPosition());
}

void RegistrationModel::ResetTransformToIdentity()
{
  ITKMatrixType matrix;
  ITKVectorType offset;

  matrix.SetIdentity();
  offset.Fill(0.0);

  // Reset the flips
  this->m_ManualParam.Flip.fill(false);
  this->SetMovingTransform(matrix, offset);
}

void RegistrationModel::MatchImageCenters()
{
  // Set the transforms so that the center voxels of the two images are matched
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  assert(layer);

  // Get the reference space - should match main image
  ImageWrapperBase::ImageBaseType *refspc = layer->GetReferenceSpace();
  ImageWrapperBase::ImageBaseType *movspc = layer->GetImageBase();


  // Compute the world coordinates of the image centers
  itk::Index<3> ciRef, ciMov;
  for(int d = 0; d < 3; d++)
    {
    ciRef[d] = refspc->GetLargestPossibleRegion().GetSize()[d] / 2;
    ciMov[d] = movspc->GetLargestPossibleRegion().GetSize()[d] / 2;
    }

  // Get the rotation center in world coordinates
  itk::Point<double, 3> cpRef, cpMov;
  refspc->TransformIndexToPhysicalPoint(ciRef, cpRef);
  movspc->TransformIndexToPhysicalPoint(ciMov, cpMov);

  // Translation should take cpRef to cpMov
  ITKMatrixType matrix;
  ITKVectorType offset;
  this->GetMovingTransform(matrix, offset);

  offset = cpMov - matrix * cpRef;

  this->SetMovingTransform(matrix, offset);
}



bool RegistrationModel::GetMovingLayerValueAndRange(unsigned long &value, RegistrationModel::LayerSelectionDomain *range)
{
  // There must be at least one layer
  LayerIterator it = m_Driver->GetCurrentImageData()->GetLayers(OVERLAY_ROLE);
  if(it.IsAtEnd())
    return false;

  // Return the active ID
  value = m_MovingLayerId;

  // Compute the range of IDs
  if(range)
    {
    range->clear();
    for(; !it.IsAtEnd(); ++it)
      {
      (*range)[it.GetLayer()->GetUniqueId()] = it.GetLayer()->GetNickname();
      }
    }

  return true;
}

void RegistrationModel::SetMovingLayerValue(unsigned long value)
{
  // Set the layer id
  m_MovingLayerId = value;

  // Update the cache
  this->UpdateManualParametersFromWrapper(true, false);

  // Fire a state change
  this->InvokeEvent(StateMachineChangeEvent());
}

bool RegistrationModel::GetInteractiveToolValue(bool &value)
{
  value = (m_Parent->GetGlobalState()->GetToolbarMode() == REGISTRATION_MODE);
  return true;
}

void RegistrationModel::SetInteractiveToolValue(bool value)
{
  if(value)
    m_Parent->GetGlobalState()->SetToolbarMode(REGISTRATION_MODE);
  else
    m_Parent->GetGlobalState()->SetToolbarMode(CROSSHAIRS_MODE);
}

bool RegistrationModel::GetEulerAnglesValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range)
{
  // Make sure that the manual parameters are valid
  if(m_ManualParam.LayerID == NOID)
    return false;

  // Assign the value trivially
  value = m_ManualParam.EulerAngles * (180.0 / vnl_math::pi);

  // Handle the range
  if(range)
    {
    range->Set(Vector3d(-180.0, -180.0, -180.0),
               Vector3d(+180.0, +180.0, +180.0),
               Vector3d(0.1, 0.1, 0.1));
    }
  return true;
}

void RegistrationModel::SetEulerAnglesValue(Vector3d value)
{
  // Update the euler angles from the widget
  m_ManualParam.EulerAngles = value * (vnl_math::pi / 180.0);

  // Update the transform using the euler angles
  this->UpdateWrapperFromManualParameters();
}

bool RegistrationModel::GetTranslationValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range)
{
  // Make sure that the manual parameters are valid
  if(m_ManualParam.LayerID == NOID)
    return false;

  // Assign the value trivially
  value = m_ManualParam.Translation;

  // Handle the range
  if(range)
    {
    *range = m_ManualParam.TranslationRange;
    }
  return true;

}

void RegistrationModel::SetTranslationValue(Vector3d value)
{
  // Update the translation vector
  m_ManualParam.Translation = value;

  // Update the transform
  this->UpdateWrapperFromManualParameters();
}

bool RegistrationModel::GetScalingValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range)
{
  // Make sure that the manual parameters are valid
  if(m_ManualParam.LayerID == NOID)
    return false;

  // Assign the value trivially
  value = m_ManualParam.Scaling;

  // Handle the range
  if(range)
    {
    range->Set(Vector3d(0.01, 0.01, 0.01), Vector3d(100.0, 100.0, 100.0), Vector3d(0.01, 0.01, 0.01));
    }
  return true;
}

void RegistrationModel::SetScalingValue(Vector3d value)
{
  // Update the translation vector
  m_ManualParam.Scaling = value;

  // Update the transform
  this->UpdateWrapperFromManualParameters();
}

bool RegistrationModel::GetFlipValue(Vector3b &value)
{
  // Make sure that the manual parameters are valid
  if(m_ManualParam.LayerID == NOID)
    return false;

  // Assign the value trivially
  value = m_ManualParam.Flip;

  return true;
}

void RegistrationModel::SetFlipValue(Vector3b value)
{
  // Update the flips
  m_ManualParam.Flip = value;

  // Update the transform
  this->UpdateWrapperFromManualParameters();
}

bool RegistrationModel::GetLogScalingValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range)
{
  // Make sure that the manual parameters are valid
  if(m_ManualParam.LayerID == NOID)
    return false;

  // Assign the value trivially
  for(int i = 0; i < 3; i++)
    value[i] = log10(m_ManualParam.Scaling[i]);

  // Handle the range
  if(range)
    {
    range->Set(Vector3d(-1., -1., -1.), Vector3d(1., 1., 1.), Vector3d(0.01, 0.01, 0.01));
    }
  return true;
}

void RegistrationModel::SetLogScalingValue(Vector3d value)
{
  // Update the translation vector
  for(int i = 0; i < 3; i++)
    m_ManualParam.Scaling[i] = pow(10.0, value[i]);

  // Update the transform
  this->UpdateWrapperFromManualParameters();
}

bool RegistrationModel::GetCoarsestResolutionLevelValueAndRange(
    int &value, ResolutionLevelDomain *domain)
{
  // Is there a moving image?
  if(!this->GetMovingLayerWrapper())
    return false;

  value = m_CoarsestResolutionLevel;
  if(domain)
    *domain = m_ResolutionLevelDomain;

  return true;
}

void RegistrationModel::SetCoarsestResolutionLevelValue(int value)
{
  m_CoarsestResolutionLevel = value;
  if(m_FinestResolutionLevel > value)
    {
    m_FinestResolutionLevelModel->SetValue(value);
    this->InvokeEvent(ModelUpdateEvent());
    }
}

bool RegistrationModel::GetFinestResolutionLevelValueAndRange(
    int &value, ResolutionLevelDomain *domain)
{
  // Is there a moving image?
  if(!this->GetMovingLayerWrapper())
    return false;

  value = m_FinestResolutionLevel;
  if(domain)
    *domain = m_ResolutionLevelDomain;

  return true;
}

void RegistrationModel::SetFinestResolutionLevelValue(int value)
{
  m_FinestResolutionLevel = value;
  if(m_CoarsestResolutionLevel < value)
    {
    m_CoarsestResolutionLevelModel->SetValue(value);
    this->InvokeEvent(ModelUpdateEvent());
    }
}

void RegistrationModel::IterationCallback(const itk::Object *object, const itk::EventObject &event)
{
  // Get the transform parameters
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformType;
  const TransformType *tran = dynamic_cast<const TransformType *>(object);

  // Apply the transform
  this->SetMovingTransform(tran->GetMatrix(), tran->GetOffset());

  // Update the last metric value
  const GreedyAPI::MetricLogType &metric_log = m_GreedyAPI->GetMetricLog();
  if(metric_log.size())
    {
    const std::vector<MultiComponentMetricReport> &last_log = metric_log.back();
    if(last_log.size())
      m_LastMetricValueModel->SetValue(last_log.back().TotalMetric);
    }

  // Fire the iteration command - this is to force the GUI to process events, instead of
  // just putting the above ModelUpdateEvent() into a bucket
  if(m_IterationCommand)
    m_IterationCommand->Execute(object, event);
}



