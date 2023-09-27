#include "digestible/digestible.h"
#include <itkImageFileReader.h>
#include <itkImage.h>
#include <itkImageRegionIterator.h>
#include <itkTimeProbe.h>
#include <MultiComponentQuantileBasedNormalizationFilter.h>

using namespace digestible;

int usage()
{
  printf("testTDigest: test t-digest quantiles (https://github.com/SpirentOrion/digestible)\n");
  printf("usage: testTDigest image quantile_between_0_and_1\n");
  return -1;
}

int main(int argc, char *argv[])
{
  if(argc < 2)
    return usage();

  double qtile = atof(argv[2]);

  typedef itk::Image<float, 3> ImageType;
  typedef itk::ImageFileReader<ImageType> ReaderType;
  typedef itk::ImageRegionIterator<ImageType> Iterator;

  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(argv[1]);
  reader->Update();
  ImageType::Pointer img = reader->GetOutput();

  tdigest digest(1000);

  itk::TimeProbe probe;
  probe.Start();
  /*
  for(Iterator it(img, img->GetBufferedRegion()); !it.IsAtEnd(); ++it)
    {
    float value = it.Get();
    digest.insert(value);
    sum+=value; n++;
    }
  digest.merge();
  */

  // Mutex for combining heaps
  std::mutex mutex;

  itk::MultiThreaderBase::Pointer mt = itk::MultiThreaderBase::New();
  mt->ParallelizeImageRegion<3>(img->GetBufferedRegion(),
        [img, &mutex, &digest](const itk::ImageRegion<3> &region)
    {
    tdigest thread_digest(1000);
    for(Iterator it(img, region); !it.IsAtEnd(); ++it)
      {
      float value = it.Get();
      thread_digest.insert(value);
      }
    thread_digest.merge();

    // Use mutex to update the global heaps
    std::lock_guard<std::mutex> guard(mutex);

    digest.insert(thread_digest);
    }, nullptr);

  float q = digest.quantile(qtile * 100.0);
  probe.Stop();

  printf("98th quantile is: %f\n", q);
  printf("Runtime: %f\n", probe.GetTotal());

  typedef itk::VectorImage<float, 3> VectorImageType;
  typedef itk::ImageFileReader<VectorImageType> VectorReaderType;

  VectorReaderType::Pointer vreader = VectorReaderType::New();
  vreader->SetFileName(argv[1]);
  vreader->Update();
  VectorImageType::Pointer vimg = vreader->GetOutput();

  typedef MultiComponentQuantileBasedNormalizationFilter<VectorImageType,VectorImageType> QFilter;
  QFilter::Pointer qf = QFilter::New();
  qf->SetInput(vimg);
  qf->SetUpperQuantile(qtile);
  qf->SetNoRemapping(true);

  itk::TimeProbe probe2;
  probe2.Start();
  qf->Update();
  probe2.Stop();

  printf("98th quantile is: %f\n", qf->GetUpperQuantileValue(0));
  printf("Runtime: %f\n", probe2.GetTotal());



  return 0;
}
