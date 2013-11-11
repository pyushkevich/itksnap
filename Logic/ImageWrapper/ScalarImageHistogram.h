#ifndef SCALARIMAGEHISTOGRAM_H
#define SCALARIMAGEHISTOGRAM_H

#include <itkDataObject.h>
#include <itkObjectFactory.h>
#include <SNAPCommon.h>

#include <vector>

/**
  A very simple histogram object. It just contains a set of bins
  counting the intensities in an image.
  */
class ScalarImageHistogram : public itk::DataObject
{
public:
  irisITKObjectMacro(ScalarImageHistogram, itk::DataObject)

  void Initialize(double vmin, double vmax, size_t nBins);
  void AddSample(double v);
  double GetBinMin(size_t iBin) const;
  double GetBinMax(size_t iBin) const;
  double GetBinCenter(size_t iBin) const;
  unsigned long GetFrequency(size_t iBin) const;
  size_t GetSize() const;

  /**
   * Add the contents of an existing histogram to the current histogram. The
   * code assumes that the histograms have been initialized with the same
   * parameters to Initialize(). This is used in multi-threaded code to build
   * histograms
   */
  void AddCompatibleHistogram(const Self &addee);

  /**
   * Apply an intensity transform to the histogram. This applies scaling and
   * shift to the bin boundaries.
   */
  void ApplyIntensityTransform(double scale, double shift);

  /**
   * Determine the largest frequency to display in the plot of the histogram.
   * Sometimes histograms have bins that are much higher than most of the
   * histogram (e.g., background voxels all zero in an image), and we want to
   * set a cutoff for the y-axis when showing such a histogram, so that the
   * bulk of the histogram is visible. We determine range here by taking the
   * 95th percentile of the histogram bin frequencies, and plotting that at
   * the level of 80% of the y-axis.
   *
   * The return value is relative to MaxFrequency, e.g., 0.1
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
  int m_BinCount;

};

inline void ScalarImageHistogram::AddSample(double v)
{
  int index = (int) (m_Scale * (v - m_FirstBinStart));

  if(index < 0)
    index = 0;
  else if(index >= m_BinCount)
    index = m_BinCount - 1;

  unsigned long k = ++m_Bins[index];

  // Update total, max frequency
  if(m_MaxFrequency < k)
    m_MaxFrequency = k;

  m_TotalSamples++;
}



#endif // SCALARIMAGEHISTOGRAM_H
