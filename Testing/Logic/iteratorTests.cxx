#include "RLEImageScanlineIterator.h"
#include <cstdlib>
#include <iostream>

extern int itkImageScanlineIteratorTest1(int argc, char *argv[]);
extern int itkIteratorTests(int argc, char *argv[]);
extern int itkImageIteratorTest(int argc, char *argv[]);
extern int itkImageIteratorsForwardBackwardTest(int argc, char *argv[]);
extern int itkImageIteratorWithIndexTest(int argc, char *argv[]);
extern int itkImageRegionConstIteratorWithOnlyIndexTest(int argc, char *argv[]);
extern int itkImageRegionIteratorTest(int argc, char *argv[]);
extern int itkRegionOfInterestImageFilterTest(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    // Each test returns EXIT_SUCCESS or EXIT_FAILURE; run them all and fail if any failed
    int nFailed = 0;
    nFailed += itkImageRegionIteratorTest(argc, argv) != EXIT_SUCCESS;
    nFailed += itkImageScanlineIteratorTest1(argc, argv) != EXIT_SUCCESS;
    nFailed += itkIteratorTests(argc, argv) != EXIT_SUCCESS;
    nFailed += itkImageIteratorTest(argc, argv) != EXIT_SUCCESS;
    nFailed += itkImageIteratorsForwardBackwardTest(argc, argv) != EXIT_SUCCESS;
    nFailed += itkImageIteratorWithIndexTest(argc, argv) != EXIT_SUCCESS;
    nFailed += itkImageRegionConstIteratorWithOnlyIndexTest(argc, argv) != EXIT_SUCCESS;
    nFailed += itkRegionOfInterestImageFilterTest(argc, argv) != EXIT_SUCCESS;
    typedef RLEImage<char, 2, char> charred2dType;
    //test size 256
    typedef RLEImage<char, 4> charred4dType; //test size 65536

    std::cout << nFailed << " iterator test(s) failed" << std::endl;
    return nFailed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}