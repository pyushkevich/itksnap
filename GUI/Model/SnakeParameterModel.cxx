#include "SnakeParameterModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SnakeParametersPreviewPipeline.h"
#include "itkImageFileReader.h"
#include "itkShiftScaleImageFilter.h"
#include "UIReporterDelegates.h"


SnakeParameterModel::SnakeParameterModel()
{
  // Create the derived models
  for(int i = 0; i < 3; i++)
    {
    m_WeightModel[i] =
        wrapIndexedGetterSetterPairAsProperty(this, i,
                                              &Self::GetWeightValueAndRange,
                                              &Self::SetWeightValue);

    m_ExponentModel[i] =
        wrapIndexedGetterSetterPairAsProperty(this, i,
                                              &Self::GetExponentValueAndRange,
                                              &Self::SetExponentValue);
    }

  m_AdvancedEquationModeModel = NewSimpleConcreteProperty(false);

  m_CasellesOrAdvancedModeModel =
      wrapGetterSetterPairAsProperty(this, &Self::GetCasellesOrAdvancedModeValue);

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
  this->m_Parent = model;

  // Listen and rebroadcast changes to the internally stored snake parameters
  Rebroadcast(m_Parent->GetGlobalState()->GetSnakeParametersModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // Set up the preview pipeline
  this->SetupPreviewPipeline();
}


void SnakeParameterModel::SetupPreviewPipeline()
{
  // Initialize the pipeline
  if(m_PreviewPipeline) delete m_PreviewPipeline;
  m_PreviewPipeline = new SnakeParametersPreviewPipeline(m_Parent->GetGlobalState());

  // Get the parent's system object
  SystemInterface *si = m_Parent->GetSystemInterface();

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
      m_ExampleImage[i]->FillBuffer(0.0f);
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
  m_PreviewPipeline->SetSnakeParameters(m_Parent->GetGlobalState()->GetSnakeParameters());
}

void SnakeParameterModel::OnUpdate()
{
  // Update the image used by the preview pipeline
  ExampleImageType *speed = this->IsRegionSnake()
      ? m_ExampleImage[1] : m_ExampleImage[0];
  if(m_PreviewPipeline->GetSpeedImage() != speed)
    m_PreviewPipeline->SetSpeedImage(speed);
  m_PreviewPipeline->SetSnakeParameters(
        m_Parent->GetGlobalState()->GetSnakeParameters());
}

bool SnakeParameterModel::IsRegionSnake()
{
  SnakeParameters param = m_Parent->GetGlobalState()->GetSnakeParameters();
  return param.GetSnakeType() == SnakeParameters::REGION_SNAKE;
}


bool SnakeParameterModel
::GetWeightValueAndRange(
    int index, double &value, NumericValueRange<double> *domain)
{
  SnakeParameters param = m_Parent->GetGlobalState()->GetSnakeParameters();
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
  SnakeParameters param = m_Parent->GetGlobalState()->GetSnakeParameters();
  switch(index)
    {
    case ALHPA:
      param.SetPropagationWeight(value); break;
    case BETA:
      param.SetCurvatureWeight(value); break;
    case GAMMA:
      param.SetAdvectionWeight(value); break;
    }
  m_Parent->GetGlobalState()->SetSnakeParameters(param);
}


bool SnakeParameterModel
::GetExponentValueAndRange(
    int index, int &value, NumericValueRange<int> *domain)
{
  // Only valid in advanced mode
  if(!this->m_AdvancedEquationModeModel->GetValue())
    return false;

  SnakeParameters param = m_Parent->GetGlobalState()->GetSnakeParameters();
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
  SnakeParameters param = m_Parent->GetGlobalState()->GetSnakeParameters();
  switch(index)
    {
    case ALHPA:
      param.SetPropagationSpeedExponent(value); break;
    case BETA:
      param.SetCurvatureSpeedExponent(value); break;
    case GAMMA:
      param.SetAdvectionSpeedExponent(value); break;
    }
  m_Parent->GetGlobalState()->SetSnakeParameters(param);
}

bool SnakeParameterModel::GetCasellesOrAdvancedModeValue()
{
  return this->GetAdvancedEquationModeModel()->GetValue() || (!this->IsRegionSnake());
}

