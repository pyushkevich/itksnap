/**
 * Use ITK library to deal with image IO.
 */

#ifndef IMAGEIO_H
#define IMAGEIO_H

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "data.h"

typedef std::vector<std::string> StringVector;

template<typename TImageType>
void ImageSizeReader(const std::string& filename, typename TImageType::SizeType& size, typename TImageType::Pointer& image)
{
  typedef itk::ImageFileReader<TImageType> ReaderType;
  typename ReaderType::Pointer reader = ReaderType::New();
  typename TImageType::RegionType region;
  try
  {
    reader->SetFileName(filename);
    reader->Update();
    image = reader->GetOutput();
    region = image->GetLargestPossibleRegion();
    size = region.GetSize();
  }
  catch( itk::ExceptionObject &err )
  {
    std::cerr << "ImageSizeReader Exception Caught !" << std::endl;
    std::cerr << err << std::endl;
    exit(1);
  }
}

template<typename TImageType, typename dataT, typename labelT>
MLData<dataT, labelT>* ImageSeriesReader(StringVector& filenames,
                       size_t& dataNum,
                       size_t& dataDim,
                       typename TImageType::SizeType& size)
{
  typedef itk::ImageRegionConstIterator<TImageType> ConstIteratorType;
  typename TImageType::Pointer image;
  typename TImageType::SizeType curSize;

  int imageDim = size.GetSizeDimension();
  dataDim = filenames.size() - 1;
  dataNum = 1;

  MLData<dataT, labelT>* data;

  int dimIdx = -1;
  for (StringVector::const_iterator filename = filenames.begin();
       filename != filenames.end(); ++filename, ++dimIdx)
    {
      ImageSizeReader<TImageType>(*filename, curSize, image);

      if (filename == filenames.begin())
        {
          size = curSize;
          for (int i = 0; i < imageDim; i++)
            {
              dataNum *= size[i];
            }
          data = new MLData<dataT, labelT>(dataNum, dataDim);
//          data = new double*[dataNum];
//          double *block = new double[dataNum*dataDim];
//          for (int i = 0; i < dataNum; i++)
//            {
//              data[i] = &block[i*dataDim];
//            }
        }
      else if (curSize != size)
        {
          std::cerr << "ImageSeriesReader size different !" << std::endl;
          exit(1);
        }

      ConstIteratorType iter(image, image->GetLargestPossibleRegion());
      size_t dataIdx = 0;
      if (filename == filenames.begin())
        {
          for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter, ++dataIdx)
            {
              data->label[dataIdx] = iter.Get();
            }
        }
      else
        {
          for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter, ++dataIdx)
            {
              data->data[dataIdx][dimIdx] = iter.Get();
            }
        }
    }
  return data;
}

template<typename TImageType>
void ImageWriter(const std::string& filename, typename TImageType::Pointer image)
{
  typedef itk::ImageFileWriter<TImageType> WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  try
  {
    writer->SetFileName(filename);
    writer->SetInput(image);
    writer->Update();
  }
  catch( itk::ExceptionObject &err )
  {
    std::cerr << "ImageWriter Exception Caught !" << std::endl;
    std::cerr << err << std::endl;
    exit(1);
  }
}

template<typename TImageType>
void ImageSeriesWriter(StringVector& filenames,
                       Matrix<double>& data,
                       typename TImageType::SizeType size)
{
  typename TImageType::IndexType index;
  index.Fill(0);
  typename TImageType::RegionType region(index, size);
  typename TImageType::Pointer image = TImageType::New();
  image->SetRegions(region);
  image->Allocate();

  typedef itk::ImageRegionIterator<TImageType> IteratorType;

  int dimIdx = 0;
  for (StringVector::const_iterator filename = filenames.begin();
       filename != filenames.end(); ++filename, ++dimIdx)
    {
      IteratorType iter(image, image->GetLargestPossibleRegion());
      long dataIdx = 0;
      for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter, ++dataIdx)
        {
          iter.Set(data[dataIdx][dimIdx]);
        }

      ImageWriter<TImageType>(*filename, image);
    }
}

#endif // IMAGEIO_H
