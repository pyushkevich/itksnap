#include <itkImage.h>
#include <itkTestingExtractSliceImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

typedef itk::Image<unsigned short, 3> Seg3DImageType;
typedef itk::Image<unsigned short, 2> Seg2DImageType;
typedef itk::ImageFileReader<Seg3DImageType> SegReaderType;
typedef itk::ImageFileWriter<Seg2DImageType> SegWriterType;
typedef itk::Testing::ExtractSliceImageFilter<Seg3DImageType, Seg2DImageType> ExType;

int main(int argc, char *argv[])
{
	SegReaderType::Pointer sr=SegReaderType::New();
	sr->SetFileName(argv[1]);
	sr->Update();
	Seg3DImageType::Pointer in = sr->GetOutput();

	ExType::Pointer ex = ExType::New();
	ex->SetInput(in);
	ex->SetDirectionCollapseToIdentity();
	Seg3DImageType::RegionType reg;	
	reg.SetSize(in->GetLargestPossibleRegion().GetSize());
	reg.SetIndex(0, 0);
	reg.SetIndex(1, 0);
	reg.SetIndex(2, 0);

	int slice = atoi(argv[4]);
	switch (argv[3][0])
	{
	case 'X':
	case 'x': reg.SetIndex(0, slice); reg.SetSize(0, 0); break;
	case 'Y':
	case 'y': reg.SetIndex(1, slice); reg.SetSize(1, 0); break;
	case 'Z':
	case 'z': reg.SetIndex(2, slice); reg.SetSize(2, 0); break;
	default: throw std::exception("Axis should be X, Y or Z"); break;
	}
	ex->SetExtractionRegion(reg);

	SegWriterType::Pointer wr = SegWriterType::New();
	wr->SetInput(ex->GetOutput());
	wr->SetFileName(argv[2]);
	wr->Update();
	return 0;
}