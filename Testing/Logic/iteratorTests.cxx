extern int itkIteratorTests(int argc, char *argv[]);
extern int itkImageIteratorTest(int argc, char *argv[]);
extern int itkImageIteratorsForwardBackwardTest(int argc, char *argv[]);
extern int itkImageIteratorWithIndexTest(int argc, char *argv[]);
extern int itkImageRegionConstIteratorWithOnlyIndexTest(int argc, char *argv[]);
extern int itkImageRegionIteratorTest(int argc, char *argv[]);
extern int itkRegionOfInterestImageFilterTest(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    itkRegionOfInterestImageFilterTest(argc, argv);
    itkIteratorTests(argc, argv);
    itkImageIteratorTest(argc, argv);
    itkImageIteratorsForwardBackwardTest(argc, argv);
    itkImageIteratorWithIndexTest(argc, argv);
    itkImageRegionConstIteratorWithOnlyIndexTest(argc, argv);
    //itkImageRegionIteratorTest(argc, argv); //RLEImage assumes buffered==requested==largest
}