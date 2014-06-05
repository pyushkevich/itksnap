/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SnakeParametersPreviewPipeline.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/19 19:15:14 $
  Version:   $Revision: 1.6 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include "SnakeParametersPreviewPipeline.h"

#include "SNAPOpenGL.h"
#include "GlobalState.h"

#include "LevelSetExtensionFilter.h"
#include "SignedDistanceFilter.h"
#include "SNAPLevelSetFunction.h"
#include "itkBSplineInterpolationWeightFunction.h"
#include "itkBSplineKernelFunction.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkNarrowBandLevelSetImageFilter.h"
#include "itkVTKImageExport.h"
#include "vtkCellArray.h"
#include "vtkContourFilter.h"
#include "vtkImageData.h"
#include "vtkImageImport.h"
#include "vtkPolyData.h"

#include "SNAPLevelSetDriver.h"
#include "PolygonScanConvert.h"

#ifndef vtkFloatingPointType
#define vtkFloatingPointType float
#endif

using namespace std;

/**
 * This private-scope class creates a 2D demo of the level set segmentation
 * The segmentation is a 2D version of the SNAP segmentation, with contours
 * extracted at each iteration. The user supplies the speed image, a set of
 * points that form the initial contour and the snake evolution parameters.
 * Then, on a timer, call OnTimerEvent() to generate a demo loop of evolving
 * contours.
 */
class LevelSetPreview2d
{
public:
  typedef itk::Image<float, 2> FloatImageType;
  typedef itk::Image<short, 2> ShortImageType;
  typedef SnakeParametersPreviewPipeline::SampledPointList CurveType;

  // Constructor
  LevelSetPreview2d()
    {
    m_Driver = NULL;
    m_SpeedImage = NULL;
    m_DriverDirty = true;
    m_ContourDirty = true;
    m_DemoLoopLength = 160;
    m_DemoLoopStep = 2;
    m_LevelSetImage = NULL;

    }

  // Destructor
  ~LevelSetPreview2d()
    { 
    if(m_Driver) delete m_Driver; 
    }

  // Timer callback, used to regenerate the current contour
  void OnTimerEvent()
    {
    // Clear the output
    m_CurrentCurve.clear();

    // If the driver is dirty, we need to create a new one
    if(m_DriverDirty && m_Driver != NULL)
      { 
      delete m_Driver;
      m_Driver = NULL;
      }

    // If the driver is null and all the necessary components exist, create it
    if(m_Driver == NULL && m_SpeedImage.IsNotNull() && m_Curve.size() > 0)
      {
      // Check if we need to allocate the level set image
      if(m_LevelSetImage.IsNull() || m_LevelSetImage->GetBufferedRegion() != 
          m_SpeedImage->GetBufferedRegion())
        {
        m_LevelSetImage = FloatImageType::New();
        m_LevelSetImage->SetRegions(m_SpeedImage->GetBufferedRegion());
        m_LevelSetImage->Allocate();
        m_ContourDirty = true;
        }

      // Check if the contour is dirty, and create a contour image
      if(m_ContourDirty)
        {
        // Scale the contour by the size of the image
        std::vector<Vector2d> points; points.reserve(m_Curve.size());
        for(CurveType::iterator it = m_Curve.begin(); it != m_Curve.end(); ++it)
          {
          points.push_back(Vector2d(
            it->x[0] *  m_LevelSetImage->GetBufferedRegion().GetSize()[0], 
            it->x[1] *  m_LevelSetImage->GetBufferedRegion().GetSize()[1]));
          }

        // Fill in the contour in the level set image
        m_LevelSetImage->FillBuffer(0.0f);
        typedef PolygonScanConvert<
          float, GL_FLOAT, std::vector<Vector2d>::iterator> ScanConvertType;
        ScanConvertType::RasterizeFilled(
          points.begin(), points.size(), m_LevelSetImage);

        // Ensure that the initial level set is zero
        typedef itk::ImageRegionIterator<FloatImageType> IteratorType;
        IteratorType it2(m_LevelSetImage, m_LevelSetImage->GetBufferedRegion());
        for(; !it2.IsAtEnd(); ++it2)
          it2.Set(it2.Get() > 0 ? -1.0 : 1.0);
        m_LevelSetImage->Modified();

        // The contour is not dirty any more
        m_ContourDirty = false;
        }

      m_Driver = new SNAPLevelSetDriver2d(
        m_LevelSetImage.GetPointer(), m_SpeedImage, m_Parameters);

      m_DriverDirty = false;
      m_DemoLoopTime = 0;
      }

    // Now that we've made sure that the driver is OK, run the demo loop
    if(m_Driver != NULL)
      {
      // Run some number of level set evolutions
      if(m_DemoLoopTime > m_DemoLoopLength)
        {
        m_DemoLoopTime = 0;
        m_Driver->Restart();
        }
      else
        {
        m_Driver->Run(m_DemoLoopStep);
        m_DemoLoopTime += m_DemoLoopStep;
        }

      // Initialize the VTK Importer
      m_VTKExporter = itk::VTKImageExport<FloatImageType>::New();
      m_VTKImporter = vtkImageImport::New();

      // Pipe the importer into the exporter (that's a lot of code)
      m_VTKImporter->SetUpdateInformationCallback(
        m_VTKExporter->GetUpdateInformationCallback());
      m_VTKImporter->SetPipelineModifiedCallback(
        m_VTKExporter->GetPipelineModifiedCallback());
      m_VTKImporter->SetWholeExtentCallback(
        m_VTKExporter->GetWholeExtentCallback());
      m_VTKImporter->SetSpacingCallback(
        m_VTKExporter->GetSpacingCallback());
      m_VTKImporter->SetOriginCallback(
        m_VTKExporter->GetOriginCallback());
      m_VTKImporter->SetScalarTypeCallback(
        m_VTKExporter->GetScalarTypeCallback());
      m_VTKImporter->SetNumberOfComponentsCallback(
        m_VTKExporter->GetNumberOfComponentsCallback());
      m_VTKImporter->SetPropagateUpdateExtentCallback(
        m_VTKExporter->GetPropagateUpdateExtentCallback());
      m_VTKImporter->SetUpdateDataCallback(
        m_VTKExporter->GetUpdateDataCallback());
      m_VTKImporter->SetDataExtentCallback(
        m_VTKExporter->GetDataExtentCallback());
      m_VTKImporter->SetBufferPointerCallback(
        m_VTKExporter->GetBufferPointerCallback());  
      m_VTKImporter->SetCallbackUserData(
        m_VTKExporter->GetCallbackUserData());  

      // Create and configure the contour filter
      m_VTKContour = vtkContourFilter::New();
      m_VTKContour->SetInputConnection(m_VTKImporter->GetOutputPort());
      m_VTKContour->ReleaseDataFlagOn();
      m_VTKContour->ComputeScalarsOff();
      m_VTKContour->ComputeGradientsOff();
      m_VTKContour->UseScalarTreeOn();
      m_VTKContour->SetNumberOfContours(1);
      m_VTKContour->SetValue(0, 0.0);

      // Generate a contour
      m_VTKExporter->SetInput(m_Driver->GetCurrentState());
      m_VTKContour->Update();

      // Get the list of points representing the evolving contour
      vtkPolyData *pd = m_VTKContour->GetOutput();
      m_CurrentCurve.reserve(pd->GetNumberOfCells() * 2);
      for(int i=0;i<pd->GetNumberOfCells();i++)
        {
        vtkFloatingPointType *pt1 = pd->GetPoint(pd->GetCell(i)->GetPointId(0));
        m_CurrentCurve.push_back(Vector2d(pt1[0] + 0.5,pt1[1] + 0.5));
        vtkFloatingPointType *pt2 = pd->GetPoint(pd->GetCell(i)->GetPointId(1));
        m_CurrentCurve.push_back(Vector2d(pt2[0] + 0.5,pt2[1] + 0.5));
        }
      }

    m_VTKImporter->Delete();
    m_VTKContour->Delete();
    }

  // Change the speed image passed as the input to the level set
  void SetSpeedImage(ShortImageType *image)
    {
    if(image != m_SpeedImage)
      {
      m_DriverDirty = true;
      m_ContourDirty = true;
      m_SpeedImage = image;
      }
    }

  // Set the initial contour curve
  void SetInitialContour(const CurveType &curve)
    {
    m_Curve = curve;
    m_ContourDirty = true;
    m_DriverDirty = true;
    }

  // Set the snake paramters
  void SetSnakeParameters(const SnakeParameters &parameters)
    {
    if(!(m_Parameters == parameters))
      {
      m_Parameters = parameters;
      m_DriverDirty = true;
      }
    }

  // Set the length of the demo loop
  void SetDemoLoopLength(unsigned int length)
    {
    m_DemoLoopLength = length;
    m_DriverDirty = true;
    }

  // Set the step size of the demo loop
  void SetDemoLoopStep(unsigned int step)
    {
    m_DemoLoopStep = step;
    m_DriverDirty = true;
    }

  // See if there is something to display
  bool IsLevelSetComputed()
    { return m_Driver != NULL && !m_DriverDirty; }

  // Get the level set image to display
  FloatImageType *GetLevelSetImage()
    { return m_Driver->GetCurrentState(); }

  // Get the evolving contour
  vector<Vector2d> &GetEvolvingContour()
    { return m_CurrentCurve; }

  // Restart the demo
  void Restart()
    { m_DriverDirty = true; m_CurrentCurve.clear(); }

private:
  // Parameters of the level set algorithm
  ShortImageType::Pointer m_SpeedImage;
  FloatImageType::Pointer m_LevelSetImage;
  CurveType m_Curve;
  SnakeParameters m_Parameters;
  unsigned int m_DemoLoopStep, m_DemoLoopLength, m_DemoLoopTime;

  // Snake evolution driver pointer
  SNAPLevelSetDriver2d *m_Driver;

  // VTK objects for computing a contour
  typedef itk::VTKImageExport<FloatImageType> ExporterType;
  itk::SmartPointer<ExporterType> m_VTKExporter;
  vtkImageImport *m_VTKImporter;
  vtkContourFilter *m_VTKContour;

  // The zero level set, as it evolves
  vector<Vector2d> m_CurrentCurve;

  bool m_DriverDirty, m_ContourDirty;
};



void 
SnakeParametersPreviewPipeline
::AnimationCallback()
{
  // Call the callback on the demo loop
  m_DemoLoop->OnTimerEvent();
}

void SnakeParametersPreviewPipeline::AnimationRestart()
{
  m_DemoLoop->Restart();
}

SnakeParametersPreviewPipeline
::SnakeParametersPreviewPipeline(GlobalState *state)
{
  // Store the global state
  m_GlobalState = state;

  // Start with a 100 interpolated points
  m_NumberOfSampledPoints = 100;

  m_ControlsModified = false;
  m_SpeedModified = false;
  m_ParametersModified = false;
  m_QuickUpdate = false;

  // Initialize the parameters
  m_Parameters = SnakeParameters::GetDefaultEdgeParameters();

  // Initialize the display filter
  m_DisplayMapper = IntensityFilterType::New();

  // TODO: the colormap from the current speed image should be used!

  // Create a new demo loop
  m_DemoLoop = new LevelSetPreview2d;
}

SnakeParametersPreviewPipeline
::~SnakeParametersPreviewPipeline()
{
  delete m_DemoLoop;
}

void
SnakeParametersPreviewPipeline
::SetControlPoints(const std::vector<Vector2d> &points)
{
  if(m_ControlPoints != points)
    {  
    // Save the points
    m_ControlPoints = points;

    // Set the flags
    m_ControlsModified = true;
    m_QuickUpdate = false;
    }
}

void 
SnakeParametersPreviewPipeline
::ChangeControlPoint(
  unsigned int index, const Vector2d &point, bool quickUpdate)
{
  // Update the point
  assert(index < m_ControlPoints.size());

  // Update the point
  m_ControlPoints[index] = point;

  // Set the flags
  m_ControlsModified = true;

  // Set the update flag
  m_QuickUpdate = quickUpdate;
}

void
SnakeParametersPreviewPipeline
::SetSpeedImage(ShortImageType *image)
{
  // Update the image internally
  if(image != m_SpeedImage)
    {
    // Set the modified flag
    m_SpeedModified = true;
    m_SpeedImage = image;

    // Create a filter to compute a gradient image
    typedef itk::GradientImageFilter<ShortImageType> GradientFilter;
    GradientFilter::Pointer filter = GradientFilter::New();

    // Set up and run the filter
    filter->SetInput(m_SpeedImage);
    m_GradientImage = filter->GetOutput();
    filter->Update();

    // Pass the image to the display functor
    m_DisplayMapper->SetInput(m_SpeedImage);
    DisplayImageType *di = m_DisplayMapper->GetOutput();
    di->Update();

    // Pass the speed image to the preview object
    m_DemoLoop->SetSpeedImage(image);
    }
}

void
SnakeParametersPreviewPipeline
::SetSnakeParameters(const SnakeParameters &parameters)
{
  // Clean up the parameters
  SnakeParameters clean = parameters;
  clean.SetClamp(false);
  clean.SetGround(0);
  clean.SetLaplacianSpeedExponent(0);
  clean.SetLaplacianWeight(0);
  clean.SetSolver(SnakeParameters::PARALLEL_SPARSE_FIELD_SOLVER);

  // Make the 2D example behave more like 3D ...
  clean.SetCurvatureWeight(5 * parameters.GetCurvatureWeight());

  // Don't waste time on nonsense
  if(m_Parameters == clean) return;
    
  // Save the parameters
  m_Parameters = clean;
  m_ParametersModified = true;

  // Pass the parameters to the demo loop
  m_DemoLoop->SetSnakeParameters(m_Parameters);
}

void 
SnakeParametersPreviewPipeline
::SetNumberOfSampledPoints(unsigned int number)
{
  if(number!=m_NumberOfSampledPoints)
    {
    m_NumberOfSampledPoints = number;
    m_ControlsModified = true;
    }
}

void
SnakeParametersPreviewPipeline
::Update()
{
  // Check what work needs to be done
  if(m_ControlsModified)
    {
    UpdateContour();
    }
  if(!m_QuickUpdate)
    {
    if(m_ControlsModified)
      {
      m_DemoLoop->SetInitialContour(GetSampledPoints());
      // UpdateLevelSet(context);
      }
    if(m_ParametersModified || m_ControlsModified)
      {
      UpdateForces();
      m_ParametersModified = false;
      }
    }

  // Clear the modified flags
  m_ControlsModified = false;

  // Also, check whether the colormap used for display has changed
  // TODO: the colormap from the current speed image should be used!
}

void 
SnakeParametersPreviewPipeline
::UpdateContour()
{
  // Create a b-spline object
  typedef itk::BSplineInterpolationWeightFunction<double,1,3> FunctionType;
  FunctionType::Pointer function = FunctionType::New();

  // Used to compute spline derivatives
  itk::BSplineKernelFunction<3>::Pointer kf3 = itk::BSplineKernelFunction<3>::New();
  itk::BSplineKernelFunction<2>::Pointer kf2 = itk::BSplineKernelFunction<2>::New();
  itk::BSplineKernelFunction<2>::Pointer kf1 = itk::BSplineKernelFunction<2>::New();

  // Initialize the sampled point array
  m_SampledPoints.clear();
  m_SampledPoints.reserve(m_NumberOfSampledPoints);
  
  int uMax = m_ControlPoints.size() - 3; 
  for(double t = 0; t < 1.0; t += 0.005)
    {
    double s = t * uMax;
    
    // The starting index
    // int si = ((int)(t * uMax)) - 1;
    int sidx = (int) floor(s - 1);
    double u = s - sidx;

    // Compute the position and derivatives of the b-spline
    Vector2d x(0.0f,0.0f);
    Vector2d xu(0.0f,0.0f);
    Vector2d xuu(0.0f,0.0f);
    for(int k=0; k < 4; k++)
      {      
      double w = kf3->Evaluate(u);
      double wu = kf2->Evaluate(u+0.5) - kf2->Evaluate(u-0.5);
      double wuu = kf1->Evaluate(u+1) + kf1->Evaluate(u-1) - 2 * kf1->Evaluate(u);
      u-=1.0;

      int idx = (uMax + sidx + k) % uMax;
      x += w * m_ControlPoints[idx];
      xu += wu * m_ControlPoints[idx];
      xuu += wuu * m_ControlPoints[idx];
      }

    // Create and save the point
    SampledPoint pt;
    pt.x = x;
    pt.t = t;
    xu.normalize();
    pt.n = Vector2d(-xu[1],xu[0]);    
    pt.PropagationForce = pt.CurvatureForce = pt.AdvectionForce = 0.0;
    pt.kappa 
      = (xu[0] * xuu[1] - xu[1] * xuu[0]) / pow(xu[0]*xu[0] + xu[1]*xu[1],1.5);

    m_SampledPoints.push_back(pt);
    }
}

void
SnakeParametersPreviewPipeline
::UpdateLevelSetFunction()
{
}

void
SnakeParametersPreviewPipeline
::UpdateLevelSet()
{
}

void
SnakeParametersPreviewPipeline
::UpdateForces()
{
  // Image interpolator types
  typedef itk::LinearInterpolateImageFunction<
    ShortImageType,double> LerpType;
  typedef itk::VectorLinearInterpolateImageFunction<
    VectorImageType,double> VectorLerpType;
  
  // Create the speed image interpolator
  LerpType::Pointer sLerp = LerpType::New();
  sLerp->SetInputImage(m_SpeedImage);

  // Create the gradient image interpolator
  VectorLerpType::Pointer gLerp = VectorLerpType::New();
  gLerp->SetInputImage(m_GradientImage);

  // Get the image dimensions
  itk::Size<2> idim = m_SpeedImage->GetBufferedRegion().GetSize();
  
  // Compute the geometry of each point
  for(unsigned int i = 0; i < m_SampledPoints.size(); i++)
    {
    // A reference so we can access the point in shorthand
    SampledPoint &p = m_SampledPoints[i];
    
    // We're done computing the geometric properties of the curve.  Now, let's
    // compute the image-related quantities.  First, convert the point to image
    // coordinates
    LerpType::ContinuousIndexType idx;
    idx[0] = idim[0] * p.x[0];
    idx[1] = idim[1] * p.x[1];

    // Get the value of the g function
    double g = sLerp->EvaluateAtContinuousIndex(idx);

    // Scale to [-1 1] range because speed is represented as a short internally
    g /= 0x7fff;

    // Get the value of the gradient
    VectorLerpType::OutputType gradG = gLerp->EvaluateAtContinuousIndex(idx);
    gradG /= 0x7fff;
    
    // Compute the propagation force component of the curve evolution
    p.PropagationForce = m_Parameters.GetPropagationWeight() 
      * pow(g,m_Parameters.GetPropagationSpeedExponent());

    // Compute the curvature force component of the curve evolution
    p.CurvatureForce = m_Parameters.GetCurvatureWeight() * p.kappa
      * pow(g,m_Parameters.GetCurvatureSpeedExponent()+1);

    // Compute the advection force component of the curve evolution
    p.AdvectionForce = - m_Parameters.GetAdvectionWeight()
      * (p.n[0] * gradG[0] + p.n[1] * gradG[1])
      * pow(g,m_Parameters.GetAdvectionSpeedExponent());    
    }
}



std::vector<Vector2d> &
SnakeParametersPreviewPipeline
::GetDemoLoopContour()
{
  return m_DemoLoop->GetEvolvingContour();
}
