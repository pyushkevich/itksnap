#include "RegistrationModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "IRISImageData.h"
#include "GlobalUIModel.h"

#include "itkEuler3DTransform.h"
#include "vnl/algo/vnl_svd.h"



const unsigned long RegistrationModel::NOID = (unsigned long)(-1);

RegistrationModel::RegistrationModel()
{
  m_MovingLayerModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetMovingLayerValueAndRange,
        &Self::SetMovingLayerValue);

  m_EulerAnglesModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetEulerAnglesValueAndRange,
        &Self::SetEulerAnglesValue);

  m_RotationCenterModel = NewSimpleConcreteProperty(Vector3ui(0, 0, 0));

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

void RegistrationModel::UpdateManualParametersFromWrapper()
{
  ImageWrapperBase *layer = this->GetMovingLayerWrapper();

  // If there is no layer, we just invalidate the parameters
  if(!layer)
    {
    // Invalidate the parameters
    m_ManualParam.LayerID = NOID;
    return;
    }

  // Get the transform
  ImageWrapperBase::ITKTransformType *transform = layer->GetITKTransform();

  // Check if the data we have is already current
  if(m_MovingLayerId == m_ManualParam.LayerID &&
     transform->GetMTime() <= m_ManualParam.UpdateTime)
    return;

  // TODO: in the future it might make more sense to stick to a single kind of
  // transform in the ImageWrapper instead of allowing different transform
  // classes. Using multiple classes seems pointless.
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformBase;
  TransformBase *tb = dynamic_cast<TransformBase *>(transform);
  TransformBase::MatrixType matrix; matrix.SetIdentity();
  TransformBase::OffsetType offset; offset.Fill(0.0);
  if(tb)
    {
    matrix = tb->GetMatrix();
    offset = tb->GetOffset();
    }

  // Decompose the transform into relevant parts
  // first, there is the polar decomposition
  //   y = Ax + b = RSx + b
  // here R is a rotation about the origin in the physical LPS coordinate system
  //     = RS(x - c) + RSc + b

  // Perform polar decomposition
  vnl_svd<double> svd(matrix.GetVnlMatrix());

  TransformBase::MatrixType rotation = svd.U() * svd.V().transpose();
  TransformBase::MatrixType shear = svd.V() * svd.W() * svd.V().transpose();

  // Get the rotation center in world coordinates
  itk::Point<double, 3> ptCenter;
  layer->GetReferenceSpace()->TransformIndexToPhysicalPoint(
        to_itkIndex(m_RotationCenterModel->GetValue()), ptCenter);

  // Compute Euler angles
  typedef itk::Euler3DTransform<double> EulerTransform;
  EulerTransform::Pointer euler = EulerTransform::New();
  euler->SetCenter(ptCenter);
  euler->SetMatrix(rotation);
  euler->SetOffset(offset);

  m_ManualParam.EulerAngles[0] = euler->GetAngleX();
  m_ManualParam.EulerAngles[1] = euler->GetAngleY();
  m_ManualParam.EulerAngles[2] = euler->GetAngleZ();

  m_ManualParam.Translation[0] = euler->GetTranslation()[0];
  m_ManualParam.Translation[1] = euler->GetTranslation()[1];
  m_ManualParam.Translation[2] = euler->GetTranslation()[2];

  m_ManualParam.LayerID = m_MovingLayerId;
  m_ManualParam.UpdateTime = transform->GetTimeStamp();

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
        to_itkIndex(m_RotationCenterModel->GetValue()), ptCenter);

  euler->SetCenter(ptCenter);
  euler->SetRotation(m_ManualParam.EulerAngles[0], m_ManualParam.EulerAngles[1], m_ManualParam.EulerAngles[2]);

  ITKVectorType translation;
  translation.SetVnlVector(m_ManualParam.Translation);
  euler->SetTranslation(translation);

  layer->SetITKTransform(layer->GetReferenceSpace(), euler);

  // Update the state of the cache
  m_ManualParam.LayerID = m_MovingLayerId;
  m_ManualParam.UpdateTime = layer->GetITKTransform()->GetTimeStamp();
}

ImageWrapperBase *RegistrationModel::GetMovingLayerWrapper()
{
  if(m_MovingLayerId == NOID)
    return NULL;

  return m_Driver->GetCurrentImageData()->FindLayer(m_MovingLayerId, false, OVERLAY_ROLE);
}

void RegistrationModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;
  m_Driver = model->GetDriver();

  // Layer changes require a lot of things to be reset
  Rebroadcast(m_Driver, LayerChangeEvent(), ModelUpdateEvent());

  // Changes to layers also affect state
  Rebroadcast(m_Driver, WrapperChangeEvent(), ModelUpdateEvent());
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
      // Is this enough? Do we need to notify
      m_MovingLayerId = NOID;
      }
    }

  // Specifically for when the main image changes
  if(main_changed)
    {
    // Reset the center of rotation to the image center
    this->ResetOnMainImageChange();
    }

  // If there is a wrapper update, update the transform parameters
  // TODO: use the modification times to minimize the amount of updates
  if(wrapper_updated || layers_changed)
    {
    // This will update the cached parameters
    this->UpdateManualParametersFromWrapper();
    }
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
}

bool RegistrationModel::GetEulerAnglesValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range)
{
  // Make sure that the manual parameters are valid
  if(m_ManualParam.LayerID == NOID)
    return false;

  // Assign the value trivially
  value = m_ManualParam.EulerAngles;

  // Handle the range
  if(range)
    {
    range->Set(Vector3d(-vnl_math::pi, -vnl_math::pi_over_2, -vnl_math::pi),
               Vector3d(+vnl_math::pi, +vnl_math::pi_over_2, +vnl_math::pi),
               Vector3d(0.01, 0.01, 0.01));
    }
  return true;
}

void RegistrationModel::SetEulerAnglesValue(Vector3d value)
{
  // Update the euler angles from the widget
  m_ManualParam.EulerAngles = value;

  // Update the transform using the euler angles
  this->UpdateWrapperFromManualParameters();
}

