#include "LayerHistogramPlotAssembly.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>
#include <vtkAxis.h>

#include "ScalarImageHistogram.h"

LayerHistogramPlotAssembly::LayerHistogramPlotAssembly()
{
  // Set up the histogram plot
  m_HistogramX = vtkSmartPointer<vtkFloatArray>::New();
  m_HistogramX->SetName("Image Intensity");
  m_HistogramY = vtkSmartPointer<vtkFloatArray>::New();
  m_HistogramY->SetName("Frequency");
  m_HistogramTable = vtkSmartPointer<vtkTable>::New();
  m_HistogramTable->AddColumn(m_HistogramX);
  m_HistogramTable->AddColumn(m_HistogramY);

  m_ReasonableScaleHeightParameter = 0.6;
  m_ReasonableScaleQuantileParameter = 0.95;
}

void LayerHistogramPlotAssembly::AddToChart(vtkChartXY *chart)
{
  m_HistogramPlot = chart->AddPlot(vtkChart::BAR);
  m_HistogramPlot->SetInputData(m_HistogramTable, 0, 1);
  m_HistogramPlot->SetColor(0.8, 0.8, 1.0);
}

/**
 * Plot the histogram with fixed limits of the frequency variable. The
 * histogram is scaled within the limits, so that the zero frequency is
 * plotted at ymin, and the largest plottable frequency is plotted at ymax.
 *
 * The largest plottable frequency can be specified as the percentage of
 * the highest frequency or if the percentage is set to zero, it is computed
 * automatically using quantiles
 */
void LayerHistogramPlotAssembly::PlotWithFixedLimits(
    const ScalarImageHistogram *histogram,
    double ymin, double ymax,
    double max_plotted_freq_fraction,
    bool log_plot)
{
  // Compute the histogram entries
  m_HistogramTable->SetNumberOfRows(histogram->GetSize());

  // Determine the maximum frequency we are going to show
  double fmax = (max_plotted_freq_fraction > 0.0)
      ? histogram->GetMaxFrequency() * max_plotted_freq_fraction
      : histogram->GetMaxFrequency() * histogram->GetReasonableDisplayCutoff(
          m_ReasonableScaleQuantileParameter,
          m_ReasonableScaleHeightParameter);

  // Change it to log if necessary
  if(log_plot)
    fmax = log10(fmax);

  // Plot the frequency values
  for(int i = 0; i < histogram->GetSize(); i++)
    {
    m_HistogramX->SetValue(i, histogram->GetBinCenter(i));

    double y = histogram->GetFrequency(i);
    if(log_plot)
      y = log10(y);

    double yplot = (y / fmax) * (ymax - ymin) + ymin;
    m_HistogramY->SetValue(i, yplot);
    }

  m_HistogramTable->Modified();
}

double LayerHistogramPlotAssembly::PlotAsEmpiricalDensity(
    const ScalarImageHistogram *histogram)
{
  // Compute the histogram entries
  m_HistogramTable->SetNumberOfRows(histogram->GetSize());

  // Figure out how to scale the histogram
  double scaleFactor = 1.0 / (histogram->GetTotalSamples() * histogram->GetBinWidth());

  // Plot the bins
  for(int i = 0; i < histogram->GetSize(); i++)
    {
    m_HistogramX->SetValue(i, histogram->GetBinCenter(i));
    m_HistogramY->SetValue(i, histogram->GetFrequency(i) * scaleFactor);
    }

  m_HistogramTable->Modified();

  // Return the y cutoff
  return histogram->GetReasonableDisplayCutoff(
        m_ReasonableScaleQuantileParameter,
        m_ReasonableScaleHeightParameter)
      * histogram->GetMaxFrequency() * scaleFactor;
}
