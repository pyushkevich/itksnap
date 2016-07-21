#include "RegistrationModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "IRISImageData.h"
#include "GlobalUIModel.h"
#include "itkQuaternionRigidTransform.h"

#include "ImageFunctions.h"
#include "itkEuler3DTransform.h"
#include "vnl/algo/vnl_svd.h"



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

  m_RotationCenter = Vector3ui(0, 0, 0);

  // Initialize the moving layer ID to be -1
  m_MovingLayerId = NOID;

  m_Driver = NULL;
  m_Parent = NULL;
}

RegistrationModel::~RegistrationModel()
{

}

void RegistrationModel::ResetOnMainImageChange()
{
  if(m_Driver->GetIRISImageData()->IsMainLoaded())
    {
    // Reset the center of rotation
    Vector3ui center;
    for(int i = 0; i < 3; i++)
      center[i] = m_Driver->GetIRISImageData()->GetMain()->GetSize()[i] / 2;
    this->SetRotationCenter(center);

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

  m_ManualParam.AffineMatrix = euler->GetMatrix().GetVnlMatrix(); // * scale_shear;
  m_ManualParam.AffineOffset = euler->GetOffset();

  // Create a new transform
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> AffineTransform;
  AffineTransform::Pointer affine = AffineTransform::New();

  // Set the matrix of the new transform y = R ( A x + b ) + z
  affine->SetMatrix(m_ManualParam.AffineMatrix);
  affine->SetOffset(m_ManualParam.AffineOffset);

  // Update the layer's transform
  layer->SetITKTransform(layer->GetReferenceSpace(), euler);

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
  ImageWrapperBase::ITKTransformType *transform = layer->GetITKTransform();

  // TODO: in the future it might make more sense to stick to a single kind of
  // transform in the ImageWrapper instead of allowing different transform
  // classes. Using multiple classes seems pointless.
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformBase;
  TransformBase *tb = dynamic_cast<TransformBase *>(transform);
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

