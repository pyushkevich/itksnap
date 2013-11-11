#include "ScalarImageHistogram.h"
#include <algorithm>

ScalarImageHistogram::ScalarImageHistogram()
{
  m_BinWidth = 0;
  m_FirstBinStart = 0;
  m_Scale = 0;
  m_MaxFrequency = 0;
  m_TotalSamples = 0;
  m_BinCount = 0;
}

ScalarImageHistogram::~ScalarImageHistogram()
{

}

void
ScalarImageHistogram
::Initialize(double vmin, double vmax, size_t nBins)
{
  // Must be more than one bin
  assert(nBins > 0);

  // min and max should be the center
  m_BinWidth = (vmax - vmin) / nBins;

  // Set the edge of the first bin
  m_FirstBinStart = vmin;

  // Set the scale factor for computing the bins
  m_Scale = 1.0 / m_BinWidth;

  // Allocate the histogram
  m_BinCount = nBins;
  m_Bins.clear();
  m_Bins.resize(m_BinCount, 0);

  m_MaxFrequency = 0;
  m_TotalSamples = 0;
}


double
ScalarImageHistogram
::GetBinMin(size_t iBin) const
{
  return m_FirstBinStart + iBin * m_BinWidth;
}

double
ScalarImageHistogram
::GetBinMax(size_t iBin) const
{
  return m_FirstBinStart + (iBin + 1) * m_BinWidth;
}

double
ScalarImageHistogram
::GetBinCenter(size_t iBin) const
{
  return m_FirstBinStart + (iBin + 0.5) * m_BinWidth;
}

unsigned long
ScalarImageHistogram
::GetFrequency(size_t iBin)  const
{
  return m_Bins[iBin];
}

size_t
ScalarImageHistogram
::GetSize() const
{
  return m_BinCount;
}

void
ScalarImageHistogram
::AddCompatibleHistogram(const Self &addee)
{
  assert(addee.m_Bins.size() == m_Bins.size());
  assert(addee.m_FirstBinStart == m_FirstBinStart);
  assert(addee.m_BinWidth == m_BinWidth);

  for(unsigned int i = 0; i < m_Bins.size(); i++)
    {
    unsigned long n = addee.m_Bins[i];
    unsigned long k = (m_Bins[i] += n);
    m_MaxFrequency = std::max(m_MaxFrequency, k);
    m_TotalSamples+=n;
    }
}

void ScalarImageHistogram::ApplyIntensityTransform(double scale, double shift)
{
  m_FirstBinStart = scale * m_FirstBinStart + shift;
  m_BinWidth = scale * m_BinWidth;
  m_Scale = 1.0 / m_BinWidth;
}

double ScalarImageHistogram::GetReasonableDisplayCutoff(double quantile, double quantile_height) const
{
  std::vector<unsigned long> binsort = m_Bins;
  std::sort(binsort.begin(), binsort.end());

  int n = binsort.size();
  int pos = std::min(std::max((int) (0.5 + quantile * (n - 1)) , 0), n - 1);
  unsigned long qval = binsort[pos];
  double cutoff_freq = std::min(qval * 1.0 / quantile_height, (double) binsort.back());
  double cutoff_fraction = cutoff_freq / m_MaxFrequency;

  // Find the closest power of 10 smaller than the cutoff
  // p <= cutoff < 10p
  double p = pow(10, std::floor(log10(cutoff_fraction)));
  double cutoff_rnd = std::ceil(cutoff_fraction / p) * p;

  return cutoff_rnd;
}





