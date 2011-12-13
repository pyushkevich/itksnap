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

  irisGetMacro(MaxFrequency, unsigned long)
  irisGetMacro(TotalSamples, unsigned long)

protected:
  ScalarImageHistogram();
  virtual ~ScalarImageHistogram();

  std::vector<unsigned long> m_Bins;

  double m_FirstBinStart, m_BinWidth, m_Scale;
  unsigned long m_MaxFrequency, m_TotalSamples;

};

#endif // SCALARIMAGEHISTOGRAM_H
