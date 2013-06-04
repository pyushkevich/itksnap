#include "SnakeParameterModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SnakeParametersPreviewPipeline.h"
#include "itkImageFileReader.h"
#include "itkShiftScaleImageFilter.h"
#include "UIReporterDelegates.h"
#include "SNAPRegistryIO.h"
#include "HistoryManager.h"
#include "SNAPImageData.h"


SnakeParameterModel::SnakeParameterModel()
{
  // Create the derived models
  for(int i = 0; i < 3; i++)
    {
    m_WeightModel[i] = wrapIndexedGetterSetterPairAsProperty(
          this, i, &Self::GetWeightValueAndRange, &Self::SetWeightValue);

    m_ExponentModel[i] = wrapIndexedGetterSetterPairAsProperty(
          this, i, &Self::GetExponentValueAndRange, &Self::SetExponentValue);
    }

  m_SpeedupFactorModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetSpeedupFactorValueAndRange, &Self::SetSpeedupFactorValue);

  m_AdvancedEquationModeModel = NewSimpleConcreteProperty(false);

  m_CasellesOrAdvancedModeModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetCasellesOrAdvancedModeValue);

  m_AnimateDemoModel = NewSimpleConcreteProperty(false);

  // Treat changes to the state as model updates
  Rebroadcast(m_AdvancedEquationModeModel, ValueChangedEvent(), ModelUpdateEvent());

  m_PreviewPipeline = NULL;
}

SnakeParameterModel::~SnakeParameterModel()
{
  if(m_PreviewPipeline) delete m_PreviewPipeline;
}

void SnakeParameterModel::SetParentModel(GlobalUIModel *model)
{
  this->m_ParentModel = model;
  this->m_ParametersModel = m_ParentModel->GetGlobalState()->GetSnakeParametersModel();

  // Listen and rebroadcast changes to the internally stored snake parameters
  Rebroadcast(m_ParametersModel, ValueChangedEvent(), ModelUpdateEvent());

  // Set up the preview pipeline
  this->SetupPreviewPipeline();
}


void SnakeParameterModel::SetupPreviewPipeline()
{
  // Initialize the pipeline
  if(m_PreviewPipeline) delete m_PreviewPipeline;
  m_PreviewPipeline = new SnakeParametersPreviewPipeline(m_ParentModel->GetGlobalState());

  // Get the parent's system object
  SystemInterface *si = m_ParentModel->GetSystemInterface();

  // Get the edge and region example image file names
  std::string fnImage[] =
    { "EdgeForcesExample.png", "RegionForcesExample.png"};

  // Typedefs
  typedef itk::Image<unsigned char, 2> InputImageType;
  typedef itk::ImageFileReader<InputImageType> ReaderType;
  typedef itk::ImageRegionIterator<InputImageType> IteratorType;
  typedef itk::ShiftScaleImageFilter<InputImageType, ExampleImageType> ScaleShiftType;

  // Load each of these images
  static const float scale_factor[] = { 1.0f / 255.0f, -2.0f / 255.0f };
  static const float shift_factor[] = { 0.0f, -127.5f };

  for(unsigned int i = 0; i < 2; i++)
    {
    try
      {
      // Initialize the image
      SmartPtr<InputImageType> imgInput = InputImageType::New();

      // Load the example image using the application resource system
      si->GetSystemInfoDelegate()->LoadResourceAsImage2D(fnImage[i], imgInput);

      // Read the image in
      ScaleShiftType::Pointer scaler = ScaleShiftType::New();
      scaler->SetScale(0x7fff * scale_factor[i]);
      scaler->SetShift(shift_factor[i]);
      scaler->SetInput(imgInput);

      scaler->Update();

      // Allocate the example image
      m_ExampleImage[i] = scaler->GetOutput();
      }
    catch(...)
      {
      // An exception occurred. We don't want to freak out about this, so
      // just print out a message
      std::cerr << "Unable to read example image " << fnImage[i]
                << "; Snake preview will me missing an image" << std::endl;

      // Initialize the image to zeros
      itk::ImageRegion<2> defregion;
      defregion.SetSize(0, 128); defregion.SetSize(1, 128);

      m_ExampleImage[i] = ExampleImageType::New();
      m_ExampleImage[i]->SetRegions(defregion);
      m_ExampleImage[i]->Allocate();
      m_ExampleImage[i]->FillBuffer(0);
      }
    }

  // Pass one of the images to the pipeline
  if(this->IsRegionSnake())
    m_PreviewPipeline->SetSpeedImage(m_ExampleImage[1]);
  else
    m_PreviewPipeline->SetSpeedImage(m_ExampleImage[0]);

  // Load the points from the registry
  std::vector<Vector2d> points;
  try
    {
    Registry regPoints;
    si->GetSystemInfoDelegate()->
        LoadResourceAsRegistry("SnakeParameterPreviewCurve.txt", regPoints);
    points = regPoints.Folder("Points").GetArray(Vector2d(0.0));
    }
  catch(...)
    {
    // Again, we don't want to freak out.
    std::cerr << "Unable to read example point data for snake preview" << std::endl;
    }

  // If there are some points in there, draw them
  if(points.size() >= 4)
    {
    // Set spline points, etc
    m_PreviewPipeline->SetControlPoints(points);
    }

  // Pass the parameters to the preview pipeline
  m_PreviewPipeline->SetSnakeParameters(m_ParametersModel->GetValue());
}

void SnakeParameterModel::OnUpdate()
{
  if(this->m_EventBucket->HasEvent(ValueChangedEvent(), m_ParametersModel))
    {
    // Send the parameters to the segmentation pipeline. This is kind of
    // stupid, because we are using a model to coordinate between two copies
    // of the parameters, but it should work
    SNAPImageData *sid = m_ParentModel->GetDriver()->GetSNAPImageData();
    if(sid && sid->IsSegmentationActive())
      {
      sid->SetSegmentationParameters(m_ParametersModel->GetValue());
      }

    // Update the speed image used by the preview pipeline
    ExampleImageType *speed = this->IsRegionSnake()
        ? m_ExampleImage[1] : m_ExampleImage[0];
    if(m_PreviewPipeline->GetSpeedImage() != speed)
      m_PreviewPipeline->SetSpeedImage(speed);
    m_PreviewPipeline->SetSnakeParameters(m_ParametersModel->GetValue());
    }
}

bool SnakeParameterModel::IsRegionSnake()
{
  SnakeParameters param = m_ParametersModel->GetValue();
  return param.GetSnakeType() == SnakeParameters::REGION_SNAKE;
}

void SnakeParameterModel::PerformAnimationStep()
{
  m_PreviewPipeline->AnimationCallback();
  InvokeEvent(DemoLoopEvent());
}

void SnakeParameterModel::ResetAnimation()
{
  m_PreviewPipeline->AnimationRestart();
}

void SnakeParameterModel::LoadParameters(const std::string &file)
{
  // Create default parameters
  SnakeParameters param = (this->IsRegionSnake())
      ? SnakeParameters::GetDefaultInOutParameters()
      : SnakeParameters::GetDefaultEdgeParameters();

  // Read parameters from file
  SNAPRegistryIO io;
  Registry regParameters(file.c_str());
  param = io.ReadSnakeParameters(regParameters, param);

  // Update the history
  m_ParentModel->GetDriver()->GetHistoryManager()->
      UpdateHistory("SnakeParameters", file, true);

  // Set the parameters
  m_ParametersModel->SetValue(param);
}

void SnakeParameterModel::SaveParameters(const std::string &file)
{
  // Load the parameters
  SnakeParameters param = m_ParametersModel->GetValue();
  SNAPRegistryIO io;
  Registry regParameters;
  io.WriteSnakeParameters(param, regParameters);
  regParameters.WriteToFile(file.c_str());

  // Update the history
  m_ParentModel->GetDriver()->GetHistoryManager()->
      UpdateHistory("SnakeParameters", file, true);
}

void SnakeParameterModel::RestoreDefaults()
{
  // Create default parameters
  SnakeParameters param = (this->IsRegionSnake())
      ? SnakeParameters::GetDefaultInOutParameters()
      : SnakeParameters::GetDefaultEdgeParameters();


  // Set the parameters
  m_ParametersModel->SetValue(param);
}


bool SnakeParameterModel
::GetWeightValueAndRange(
    int index, double &value, NumericValueRange<double> *domain)
{
  SnakeParameters param = m_ParametersModel->GetValue();
  if(index == ALHPA)
    {
    value = param.GetPropagationWeight();
    if(domain)
      domain->Set(this->IsRegionSnake() ? 0.0 : -1.0, 1.0, 0.01);
    return true;
    }
  else if(index == BETA)
    {
    value = param.GetCurvatureWeight();
    if(domain)
      domain->Set(0.0, 1.0, 0.01);
    return true;
    }
  else if(index == GAMMA && this->GetCasellesOrAdvancedModeValue())
    {
    value = param.GetAdvectionWeight();
    if(domain)
      domain->Set(0.0, 5.0, 0.05);
    return true;
    }
  return false;
}


void
SnakeParameterModel
::SetWeightValue(int index, double value)
{
  SnakeParameters param = m_ParametersModel->GetValue();
  switch(index)
    {
    case ALHPA:
      param.SetPropagationWeight(value); break;
    case BETA:
      param.SetCurvatureWeight(value); break;
    case GAMMA:
      param.SetAdvectionWeight(value); break;
    }
  m_ParametersModel->SetValue(param);
}


bool SnakeParameterModel
::GetExponentValueAndRange(
    int index, int &value, NumericValueRange<int> *domain)
{
  // Only valid in advanced mode
  if(!this->m_AdvancedEquationModeModel->GetValue())
    return false;

  SnakeParameters param = m_ParametersModel->GetValue();
  if(index == ALHPA)
    {
    value = param.GetPropagationSpeedExponent();
    if(domain)
      domain->Set(0, 2, 1);
    return true;
    }
  else if(index == BETA)
    {
    value = param.GetCurvatureSpeedExponent();
    if(domain)
      domain->Set(0, 2, 1);
    return true;
    }
  else if(index == GAMMA)
    {
    value = param.GetAdvectionSpeedExponent();
    if(domain)
      domain->Set(0, 2, 1);
    return true;
    }
  return false;
}


void
SnakeParameterModel
::SetExponentValue(int index, int value)
{
  SnakeParameters param = m_ParametersModel->GetValue();
  switch(index)
    {
    case ALHPA:
      param.SetPropagationSpeedExponent(value); break;
    case BETA:
      param.SetCurvatureSpeedExponent(value); break;
    case GAMMA:
      param.SetAdvectionSpeedExponent(value); break;
    }
  m_ParametersModel->SetValue(param);
}

bool
SnakeParameterModel
::GetSpeedupFactorValueAndRange(double &value, NumericValueRange<double> *domain)
{
  SnakeParameters param = m_ParametersModel->GetValue();
  if(param.GetAutomaticTimeStep())
    value = 1;
  else
    value = param.GetTimeStepFactor();

  if(domain)
    domain->Set(1.0, 10.0, 0.25);

  return true;
}

void
SnakeParameterModel
::SetSpeedupFactorValue(double value)
{
  SnakeParameters param = m_ParametersModel->GetValue();

  if(value == 1.0)
    {
    param.SetAutomaticTimeStep(true);
    param.SetTimeStepFactor(1.0f);
    }
  else
    {
    param.SetAutomaticTimeStep(false);
    param.SetTimeStepFactor((float) value);
    }

  m_ParametersModel->SetValue(param);
}

bool SnakeParameterModel::GetCasellesOrAdvancedModeValue()
{
  return this->GetAdvancedEquationModeModel()->GetValue() || (!this->IsRegionSnake());
}

