#ifndef LAYERHISTOGRAMPLOTASSEMBLY_H
#define LAYERHISTOGRAMPLOTASSEMBLY_H

#include "SNAPCommon.h"

class vtkActor;
class vtkChartXY;
class vtkFloatArray;
class vtkPlot;
class vtkTable;

class ScalarImageHistogram;

#include <vtkSmartPointer.h>

/**
 * @brief The HistogramPlotAssembly class
 * This class creates vtk objects needed to plot a histogram of a
 * SNAP image layer using VTK charts
 */
class LayerHistogramPlotAssembly
{
public:
  LayerHistogramPlotAssembly();

  void AddToChart(vtkChartXY *chart);

  /** See ScalarImageHistogram::GetReasonableDisplayCutoff */
  irisGetSetMacro(ReasonableScaleQuantileParameter, double)

  /** See ScalarImageHistogram::GetReasonableDisplayCutoff */
  irisGetSetMacro(ReasonableScaleHeightParameter, double)

  /**
   * Plot the histogram with fixed limits of the frequency variable. The
   * histogram is scaled within the limits, so that the zero frequency is
   * plotted at ymin, and the largest plottable frequency is plotted at ymax.
   *
   * The largest plottable frequency can be specified as the percentage of
   * the highest frequency or if the percentage is set to zero, it is computed
   * automatically using quantiles
   */
  void PlotWithFixedLimits(
      const ScalarImageHistogram *histogram,
      double ymin, double ymax,
      double max_plotted_freq_fraction = 0.0,
      bool log_plot = false);

  /**
   * Plot the histogram as an empirical probability density. The heights
   * (y coordinates) of the bars are scaled so that the histogram integrates
   * to one. The reasonable max for the y coordinate (one that tries to
   * assure that outlier bins don't cause the rest of the histogram to shrink)
   * is returned.
   */
  double PlotAsEmpiricalDensity(const ScalarImageHistogram *histogram);

protected:
  vtkSmartPointer<vtkTable> m_HistogramTable;
  vtkSmartPointer<vtkPlot> m_HistogramPlot;
  vtkSmartPointer<vtkFloatArray> m_HistogramX, m_HistogramY;

  double m_ReasonableScaleQuantileParameter;
  double m_ReasonableScaleHeightParameter;
};

#endif // LAYERHISTOGRAMPLOTASSEMBLY_H
