#ifndef SCALARIMAGEHISTOGRAM_H
#define SCALARIMAGEHISTOGRAM_H

#include <itkObject.h>
#include <itkObjectFactory.h>
#include <SNAPCommon.h>

#include <vector>

/**
  A very simple histogram object. It just contains a set of bins
  counting the intensities in an image.
  */
class ScalarImageHistogram : public itk::Object
{
public:
  irisITKObjectMacro(ScalarImageHistogram, itk::Object)

  void Initialize(double vmin, double vmax, size_t nBins);
  void AddSample(double v);
  double GetBinMin(size_t iBin) const;
  double GetBinMax(size_t iBin) const;
  double GetBinCenter(size_t iBin) const;
  unsigned long GetFrequency(size_t iBin) const;
  size_t GetSize() const;

  /**
   * Determine the largest frequency to display in the plot of the histogram.
   * Sometimes histograms have bins that are much higher than most of the
   * histogram (e.g., background voxels all zero in an image), and we want to
   * set a cutoff for the y-axis when showing such a histogram, so that the
   * bulk of the histogram is visible. We determine range here by taking the
   * 95th percentile of the histogram bin frequencies, and plotting that at
   * the level of 80% of the y-axis.
   */
  double GetReasonableDisplayCutoff(double quantile=0.95, double quantile_height=0.80) const;

  irisGetMacro(MaxFrequency, unsigned long)
  irisGetMacro(TotalSamples, unsigned long)
  irisGetMacro(BinWidth, double)

protected:
  ScalarImageHistogram();
  virtual ~ScalarImageHistogram();

  std::vector<unsigned long> m_Bins;

  double m_FirstBinStart, m_BinWidth, m_Scale;
  unsigned long m_MaxFrequency, m_TotalSamples;

};

#endif // SCALARIMAGEHISTOGRAM_H
