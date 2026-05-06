#include <itkImageFileReader.h>
#include <itkImage.h>
#include <itkImageRegionIterator.h>
#include <itkImageRandomConstIteratorWithIndex.h>
#include <itkTimeProbe.h>
#include <MultiComponentQuantileBasedNormalizationFilter.h>
#include "TDigestImageFilter.h"

#include "tdigest_apache/tdigest.hpp"

int usage()
{
  printf("testTDigest: test t-digest quantiles (https://github.com/apache/datasketches-cpp)\n");
  printf("usage: testTDigest <image> <quantile> [target_value] [tolerance]\n");
  return -1;
}

bool
test_qtile_result(const char     *resulttype,
                  double          qtile,
                  double          q,
                  itk::TimeProbe &probe,
                  double          target_value,
                  double          iqr,
                  double          tolerance)
{
  double delta = std::abs(q - target_value);
  bool ok = delta <= iqr * tolerance;

  printf("%s: \n", resulttype);
  printf("  %4.4f-th quantile approximated as: %f\n", qtile, q);
  printf("  error (%% of IQR): %4.6f\n", 100 * delta / iqr);
  printf("  within tolerance: %s\n", ok ? "yes" : "no");
  printf("  runtime: %f\n", probe.GetTotal());

  return ok;
}

int main(int argc, char *argv[])
{
  if(argc < 2)
    return usage();

  double qtile = atof(argv[2]);
  double tolerance = 0.05;

  int digest_size = 1000;
  bool success = true;

  typedef itk::Image<float, 3> ImageType;
  typedef itk::ImageFileReader<ImageType> ReaderType;
  typedef itk::ImageRegionIterator<ImageType> Iterator;

  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(argv[1]);
  reader->Update();
  ImageType::Pointer img = reader->GetOutput();


  // ***********************************************
  // ** REFERENCE IMPLEMENTATION
  // ***********************************************
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

  itk::TimeProbe probe_filter;
  probe_filter.Start();
  qf->Update();
  double q_reference = qf->GetUpperQuantileValue(0);
  probe_filter.Stop();

  // Compute the inter-quartile range of the image
  qf->SetLowerQuantile(0.25);
  qf->SetUpperQuantile(0.75);
  qf->Update();
  double iqr = qf->GetUpperQuantileValue(0) - qf->GetLowerQuantileValue(0);

  printf("Reference quantile computation: \n");
  printf("  %4.4f-th quantile true value: %f\n", qtile, q_reference);
  printf("  Tolerance (%.2f%% of IQR): %f\n", 100*tolerance, tolerance * iqr);
  printf("  runtime: %f\n", probe_filter.GetTotal());

  // ***********************************************
  // ** SINGLE-THREAD APACHE TDIGEST
  // ***********************************************
  datasketches::tdigest_double td(digest_size);
  itk::TimeProbe probe_direct;
  probe_direct.Start();
  for (Iterator it(img, img->GetBufferedRegion()); !it.IsAtEnd(); ++it)
  {
    double value = it.Get();
    td.update(value);
  }
  double q_direct = td.get_quantile(qtile);
  probe_direct.Stop();

  // Check and report result
  success &= test_qtile_result(
    "Direct tdigest computation", qtile, q_direct, probe_direct, q_reference, iqr, tolerance);

  // ***********************************************
  // ** MULTITHREAD APACHE TDIGEST
  // ***********************************************
  datasketches::tdigest_double digest_threaded(digest_size);
  itk::TimeProbe probe_threaded;
  probe_threaded.Start();

  // Mutex for combining heaps
  std::mutex mutex;

  itk::MultiThreaderBase::Pointer mt = itk::MultiThreaderBase::New();
  int n_threads = 0;
  mt->ParallelizeImageRegion<3>(img->GetBufferedRegion(),
        [img, &mutex, digest_size, &digest_threaded, &n_threads](const itk::ImageRegion<3> &region)
    {
    datasketches::tdigest_double td(digest_size);
    for(Iterator it(img, region); !it.IsAtEnd(); ++it)
      {
      float value = it.Get();
      td.update(value);
      }

    // Use mutex to update the global heaps
    std::lock_guard<std::mutex> guard(mutex);

    digest_threaded.merge(td);
    n_threads++;
    }, nullptr);

  double q_threaded = digest_threaded.get_quantile(qtile);
  probe_threaded.Stop();

  // Check and report result
  char buffer[256];
  snprintf(buffer, 256, "Threaded tdigest computation (%d threads)", n_threads);
  success &= test_qtile_result(
    buffer, qtile, q_threaded, probe_threaded, q_reference, iqr, tolerance);

  // ***********************************************
  // ** APACHE TDIGEST-BASED ITK FILTER
  // ***********************************************
  itk::TimeProbe probe_tdf;
  using TDFilter = TDigestImageFilter<ImageType>;
  probe_tdf.Start();
  TDFilter::Pointer tdf = TDFilter::New();
  tdf->SetInput(img);
  tdf->Update();
  probe_tdf.Stop();
  double q_tdf = tdf->GetTDigest()->GetImageQuantile(qtile);
  success &= test_qtile_result(
    "TDigestImageFilter computation", qtile, q_tdf, probe_tdf, q_reference, iqr, tolerance);

  return success ? 0 : 1;
}
