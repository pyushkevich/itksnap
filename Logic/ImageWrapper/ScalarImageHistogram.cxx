#include "ScalarImageHistogram.h"

ScalarImageHistogram::ScalarImageHistogram()
{
  m_BinWidth = 0;
  m_FirstBinStart = 0;
  m_Scale = 0;
  m_MaxFrequency = 0;
  m_TotalSamples = 0;
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
  m_Bins.resize(nBins, 0);

  m_MaxFrequency = 0;
  m_TotalSamples = 0;
}

void
ScalarImageHistogram
::AddSample(double v)
{
  int index = (int) (m_Scale * (v - m_FirstBinStart));

  if(index < 0) index = 0;
  else if(index >= (int) m_Bins.size())
    index = m_Bins.size() - 1;

  unsigned long k = ++m_Bins[index];

  // Update total, max frequency
  m_MaxFrequency = std::max(m_MaxFrequency, k);
  m_TotalSamples++;
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
  return m_Bins.size();
}





