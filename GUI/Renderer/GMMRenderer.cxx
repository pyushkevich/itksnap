#include "GMMRenderer.h"

#include "SnakeWizardModel.h"
#include "IRISApplication.h"
#include "GlobalUIModel.h"
#include "UnsupervisedClustering.h"
#include "GaussianMixtureModel.h"
#include "ImageWrapperBase.h"
#include "LayerHistogramPlotAssembly.h"
#include "SNAPCommon.h"
#include "ScalarImageHistogram.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkDoubleArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
#include "vtkGenericOpenGLRenderWindow.h"

unsigned int GMMRenderer::NUM_POINTS = 200;

GMMRenderer::GMMRenderer()
{
  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();
  m_Chart->SetActionToButton(vtkChartXY::PAN, vtkContextMouseEvent::LEFT_BUTTON);
  m_Chart->SetActionToButton(vtkChartXY::ZOOM, vtkContextMouseEvent::RIGHT_BUTTON);
  m_Chart->SetShowLegend(true);

  // Create the histogram assembly
  m_HistogramAssembly = new LayerHistogramPlotAssembly();

  // Add the chart to the renderer
  m_ContextView->GetScene()->AddItem(m_Chart);

  // Set the background to white
  m_BackgroundColor.fill(1.0);

  // Customize the render window
  this->m_RenderWindow->SetMultiSamples(0);
  this->m_RenderWindow->SetLineSmoothing(1);
  this->m_RenderWindow->SetPolygonSmoothing(1);
}


void GMMRenderer::SetModel(SnakeWizardModel *model)
{
  m_Model = model;

  // Rebroadcast the relevant events from the model in order for the
  // widget that uses this renderer to cause an update
  Rebroadcast(model, SnakeWizardModel::GMMModifiedEvent(), ModelUpdateEvent());

  // Also listen to changes in the plotted component
  Rebroadcast(model->GetClusterPlottedComponentModel(),
              ValueChangedEvent(), ModelUpdateEvent());
}

void GMMRenderer::OnUpdate()
{
  this->UpdatePlotValues();
}

void GMMRenderer::UpdatePlotValues()
{
  int comp = m_Model->GetClusterPlottedComponentModel()->GetValue();

  // Is clustering support enabled?
  IRISApplication *app = m_Model->GetParent()->GetDriver();
  UnsupervisedClustering *uc = app->GetClusteringEngine();
  if(uc)
    {
    // Get the component of interest
    ScalarImageWrapperBase *cw =
        m_Model->GetLayerAndIndexForNthComponent(comp).ComponentWrapper;

    // Get the range of the component
    double cmin = cw->GetImageMinNative(), cmax = cw->GetImageMaxNative();

    // Remove all plots from the chart
    m_Chart->ClearPlots();

    // Add and set up the histogram plot
    const ScalarImageHistogram *hist = cw->GetHistogram(0);
    m_HistogramAssembly->AddToChart(m_Chart);
    double ymax = m_HistogramAssembly->PlotAsEmpiricalDensity(hist);

    // Get tehe GMM
    GaussianMixtureModel *gmm = uc->GetMixtureModel();
    int ng = gmm->GetNumberOfGaussians();

    // Create the plot table
    vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();

    // Create the x coordinate array. This is an array of equally sampled
    // coordinate values, plus the mean of each cluster, which is given its
    // own sample so we can render clusters that are like delta functions.
    vtkSmartPointer<vtkDoubleArray> dax = vtkSmartPointer<vtkDoubleArray>::New();
    dax->SetName("Intensity");
    table->AddColumn(dax);

    // Create the y arrays
    std::vector<vtkSmartPointer<vtkDoubleArray> > day(ng);
    for(int i = 0; i < ng; i++)
      {
      std::ostringstream oss; oss << "Cluster " << (i + 1);
      day[i] = vtkSmartPointer<vtkDoubleArray>::New();
      day[i]->SetName(oss.str().c_str());
      table->AddColumn(day[i]);
      }

    // Use the set mechanism to sort coordinate values
    vnl_vector<double> xreg = linspace(cmin, cmax, NUM_POINTS);
    std::set<double> xset;
    for(int k = 0; k < xreg.size(); k++)
      xset.insert(xreg[k]);
    for(int i = 0; i < ng; i++)
      xset.insert(m_Model->GetClusterNativeMean(i, comp));

    // Set the number of data points
    unsigned int nsam = xset.size();
    table->SetNumberOfRows(nsam);

    // Copy the x values to the array
    std::copy(xset.begin(), xset.end(), dax->GetPointer(0));

    // Plot the Gaussian for each of the clusters. The marginal Gaussian
    // PDF is just the univariate PDF with the mean and variance of that
    // component
    for(int i = 0; i < ng; i++)
      {
      // Get the i'th Gaussian as a marginal distribution
      double mean = m_Model->GetClusterNativeMean(i, comp);
      double variance = m_Model->GetClusterNativeCovariance(i, comp, comp);
      Gaussian g_marginal(1);
      g_marginal.SetMean(vnl_vector<double>(&mean, 1));
      g_marginal.SetCovariance(vnl_matrix<double>(&variance, 1, 1));

      // Set of plots, one for each mixture in the model
      vtkPlot *plot = m_Chart->AddPlot(vtkChart::LINE);
      plot->SetInputData(table, 0, i+1);

      // Compute the mixture's marginal PDF
      for(int k = 0; k < nsam; k++)
        {
        double t = dax->GetTuple1(k);
        if(t == mean && variance == 0)
          day[i]->SetTuple1(k, 10 / (cmax - cmin)); // delta function
        else
          {
          double density = g_marginal.EvaluatePDF(&t) * gmm->GetWeight(i);
          day[i]->SetTuple1(k, density);
          }
        }

      // Configure the plot
      Vector3d rgb = ColorLabelTable::GetDefaultColorLabel(i+1).GetRGBAsDoubleVector();
      plot->SetColor(rgb[0], rgb[1], rgb[2]);
      plot->GetXAxis()->SetBehavior(vtkAxis::FIXED);
      plot->GetXAxis()->SetRange(cmin - hist->GetBinWidth(), cmax + hist->GetBinWidth());
      plot->GetXAxis()->SetTitle("intensity");
      plot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
      plot->GetYAxis()->SetRange(0, ymax);
      plot->GetYAxis()->SetTitle("probability density");

      if(gmm->IsForeground(i))
        plot->SetWidth(2.0);
      else
        plot->SetWidth(1.0);
      }
    }
}

void GMMRenderer::OnDevicePixelRatioChange(int old_ratio, int new_ratio)
{
  this->UpdateChartDevicePixelRatio(m_Chart, old_ratio, new_ratio);
}
