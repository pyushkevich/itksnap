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

void RegistrationModel::UpdateManualParametersFromWrapper(bool force_update)
{
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();

  // If there is no layer, we just invalidate the parameters
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

  // Decompose the transform into relevant parts
  // first, there is the polar decomposition
  //   y = Ax + b = RSx + b
  // here R is a rotation about the origin in the physical LPS coordinate system
  //     = RS(x - c) + RSc + b

  // Perform polar decomposition
  vnl_svd<double> svd(m_ManualParam.AffineMatrix.GetVnlMatrix());

  ITKMatrixType rotation = svd.U() * svd.V().transpose();

  // Get the rotation center in world coordinates
  itk::Point<double, 3> ptCenter;
  layer->GetReferenceSpace()->TransformIndexToPhysicalPoint(
        to_itkIndex(m_RotationCenter), ptCenter);

  // Compute Euler angles
  typedef itk::Euler3DTransform<double> EulerTransform;
  EulerTransform::Pointer euler = EulerTransform::New();
  euler->SetCenter(ptCenter);
  euler->SetMatrix(rotation);
  euler->SetOffset(m_ManualParam.AffineOffset);

  m_ManualParam.EulerAngles[0] = euler->GetAngleX();
  m_ManualParam.EulerAngles[1] = euler->GetAngleY();
  m_ManualParam.EulerAngles[2] = euler->GetAngleZ();

  m_ManualParam.Translation[0] = euler->GetTranslation()[0];
  m_ManualParam.Translation[1] = euler->GetTranslation()[1];
  m_ManualParam.Translation[2] = euler->GetTranslation()[2];

  // The scaling factors are the diagonal entries of the W matrix
  m_ManualParam.Scaling[0] = svd.W()(0,0);
  m_ManualParam.Scaling[1] = svd.W()(1,1);
  m_ManualParam.Scaling[2] = svd.W()(2,2);

  // The shearing rotation matrix can be represented as Euler angles or quaternion
  // or whatever, but since we never present these parameters to the user, it is easier
  // to just cache it in matrix format
  m_ManualParam.ShearingMatrix = svd.V();

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

void RegistrationModel::UpdateWrapperFromManualParameters()
{
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();
  assert(layer);

  // Create a new euler transform
  typedef itk::Euler3DTransform<double> EulerTransform;
  EulerTransform::Pointer euler = EulerTransform::New();

  // Get the rotation center in world coordinates
  itk::Point<double, 3> ptCenter;
  layer->GetReferenceSpace()->TransformIndexToPhysicalPoint(
        to_itkIndex(m_RotationCenter), ptCenter);

  euler->SetCenter(ptCenter);
  euler->SetRotation(m_ManualParam.EulerAngles[0], m_ManualParam.EulerAngles[1], m_ManualParam.EulerAngles[2]);

  ITKVectorType translation;
  translation.SetVnlVector(m_ManualParam.Translation);
  euler->SetTranslation(translation);


  // Compute the scaling and shearing matrix
  vnl_matrix_fixed<double, 3, 3> scaling;
  scaling.fill(0.0);
  scaling.set_diagonal(m_ManualParam.Scaling);

  vnl_matrix_fixed<double, 3, 3> scale_shear =
      m_ManualParam.ShearingMatrix * scaling * m_ManualParam.ShearingMatrix.transpose();

  m_ManualParam.AffineMatrix = euler->GetMatrix().GetVnlMatrix() * scale_shear;
  m_ManualParam.AffineOffset = euler->GetOffset();

  // Create a new transform
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> AffineTransform;
  AffineTransform::Pointer affine = AffineTransform::New();

  // Set the matrix of the new transform y = R ( A x + b ) + z
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
  this->UpdateManualParametersFromWrapper(true);
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
  this->UpdateManualParametersFromWrapper();
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

ImageWrapperBase *RegistrationModel::GetMovingLayerWrapper()
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
    ImageWrapperBase *seg = this->GetParent()->GetDriver()->GetCurrentImageData()->GetSegmentation();
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
  // Get the current transform to store
  ITKMatrixType matrix;
  ITKVectorType offset;

  // Load based on the transform type
  if(format == FORMAT_C3D)
    {
    // Read the matrix file
    vnl_matrix<double> Q(4, 4);
    std::ifstream fin(filename);
    for(size_t i = 0; i < 4; i++)
      for(size_t j = 0; j < 4; j++)
        if(fin.good())
          {
          fin >> Q[i][j];
          }
        else
          {
          fin.close();
          throw IRISException("Unable to read 4x4 matrix from file %s", filename);
          }
    fin.close();

    // Flip between RAS and LPS
    Q(2,0) *= -1; Q(2,1) *= -1;
    Q(0,2) *= -1; Q(1,2) *= -1;
    Q(0,3) *= -1; Q(1,3) *= -1;

    // Populate the matrix and offset components
    matrix.GetVnlMatrix() = Q.extract(3, 3, 0, 0);
    offset.SetVnlVector(Q.get_column(3).extract(3, 0));
    }
  else // (format == FORMAT_ITK)
    {
    // The transform format we expect
    typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformBase;

    // Create a factory for reading base transforms
    itk::TransformFactory<TransformBase>::RegisterTransform();

    // Read the trasnform
    itk::TransformFileReader::Pointer reader = itk::TransformFileReader::New();
    reader->SetFileName(filename);
    reader->Update();

    // Get the transform pointer
    TransformBase *tran = NULL;
    if(reader->GetTransformList()->size())
      tran = dynamic_cast<TransformBase *>(reader->GetTransformList()->front().GetPointer());

    if(tran == NULL)
      throw IRISException("Failed to read linear transform from file %s", filename);

    // Assign the matrix/offset
    matrix = tran->GetMatrix();
    offset = tran->GetOffset();
    }

  // Update the history
  this->GetParent()->GetSystemInterface()
      ->GetHistoryManager()->UpdateHistory("AffineTransform", filename, true);

  // Now, the transform tran should hold our matrix and offset
  this->SetMovingTransform(matrix, offset);
}

void RegistrationModel::SaveTransform(const char *filename, TransformFormat format)
{
  // Get the current transform to store
  ITKMatrixType matrix;
  ITKVectorType offset;
  this->GetMovingTransform(matrix, offset);

  // Save based on the transform type
  if(format == FORMAT_C3D)
    {
    // Create a 4x4 matrix in RAS coordinate space
    vnl_matrix<double> Q(4, 4);
    vnl_matrix<double> v(offset.GetVnlVector().data_block(), 3, 1);
    Q.set_identity();
    Q.update(matrix.GetVnlMatrix(), 0, 0);
    Q.update(v, 0, 3);

    // Flip between RAS and LPS
    Q(2,0) *= -1; Q(2,1) *= -1;
    Q(0,2) *= -1; Q(1,2) *= -1;
    Q(0,3) *= -1; Q(1,3) *= -1;

    // Write the matrix
    std::ofstream matrixFile;
    matrixFile.open(filename);
    matrixFile << Q;
    matrixFile.close();
    }
  else // (format == FORMAT_ITK)
    {
    typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformBase;
    TransformBase::Pointer transform = TransformBase::New();

    transform->SetMatrix(matrix);
    transform->SetOffset(offset);

    itk::TransformFileWriter::Pointer writer = itk::TransformFileWriter::New();
    writer->SetFileName(filename);
    writer->SetInput(transform);
    writer->Update();
    }

  // Update the history
  this->GetParent()->GetSystemInterface()
      ->GetHistoryManager()->UpdateHistory("AffineTransform", filename, true);
}

const std::vector<std::vector<double> > &RegistrationModel::GetRegistrationMetricLog() const
{
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
    this->UpdateManualParametersFromWrapper();
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
  this->UpdateManualParametersFromWrapper();

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
    range->Set(Vector3d(-180.0, -90.0, -180.0),
               Vector3d(+180.0, +90.0, +180.0),
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
    const std::vector<double> &last_log = metric_log.back();
    if(last_log.size())
      m_LastMetricValueModel->SetValue(last_log.back());
    }

  // Fire the iteration command - this is to force the GUI to process events, instead of
  // just putting the above ModelUpdateEvent() into a bucket
  if(m_IterationCommand)
    m_IterationCommand->Execute(object, event);
}



