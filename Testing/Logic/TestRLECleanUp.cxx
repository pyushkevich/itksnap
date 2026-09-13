/**
 * TestRLECleanUp.cxx
 *
 * Verifies RLEImage::CleanUp(), which is called when on-the-fly cleanup is
 * turned back on. With cleanup off, setting pixels leaves adjacent run-length
 * segments that hold the same value. CleanUp() must merge them in every line
 * of the image without changing any pixel value.
 */

#include "RLEImage.h"

#include <cstdlib>
#include <iostream>

using RLEImageType = RLEImage<short>;

// Pixel value written at (x, y, z) in the test pattern
static short
patternValue(int x, int y, int z)
{
  return (x >= 2 && x < 2 + y + z) ? 7 : 0;
}

int
main()
{
  RLEImageType::Pointer  image = RLEImageType::New();
  RLEImageType::SizeType size = { { 12, 4, 3 } };
  image->SetRegions(RLEImageType::RegionType(size));
  image->Allocate();
  image->SetOnTheFlyCleanup(false);

  // Write the pattern, then overwrite single pixels with the value they
  // already hold after a detour, which leaves fragmented segments behind
  for (int z = 0; z < 3; z++)
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 12; x++)
      {
        RLEImageType::IndexType idx = { { x, y, z } };
        image->SetPixel(idx, 1);
        image->SetPixel(idx, patternValue(x, y, z));
      }

  // Every line of the buffer, before and after cleanup
  RLEImageType::BufferType::Pointer buffer = image->GetBuffer();
  const RLEImageType::RLLine       *lines = buffer->GetBufferPointer();
  const itk::SizeValueType          nLines = buffer->GetPixelContainer()->Size();

  itk::SizeValueType segmentsBefore = 0;
  for (itk::SizeValueType i = 0; i < nLines; i++)
    segmentsBefore += lines[i].size();

  image->SetOnTheFlyCleanup(true);

  int                errors = 0;
  itk::SizeValueType segmentsAfter = 0;
  for (itk::SizeValueType i = 0; i < nLines; i++)
  {
    const RLEImageType::RLLine &line = lines[i];
    segmentsAfter += line.size();

    int total = 0;
    for (size_t s = 0; s < line.size(); s++)
    {
      total += line[s].first;
      if (s > 0 && line[s].second == line[s - 1].second)
      {
        std::cerr << "Line " << i << " still has adjacent segments with value " << line[s].second
                  << std::endl;
        errors++;
      }
    }
    if (total != 12)
    {
      std::cerr << "Line " << i << " covers " << total << " pixels, expected 12" << std::endl;
      errors++;
    }
  }

  for (int z = 0; z < 3; z++)
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 12; x++)
      {
        RLEImageType::IndexType idx = { { x, y, z } };
        if (image->GetPixel(idx) != patternValue(x, y, z))
        {
          std::cerr << "Pixel " << idx << " is " << image->GetPixel(idx) << ", expected "
                    << patternValue(x, y, z) << std::endl;
          errors++;
        }
      }

  std::cout << "Segments before cleanup: " << segmentsBefore
            << ", after cleanup: " << segmentsAfter << std::endl;

  if (segmentsBefore <= segmentsAfter)
  {
    std::cerr << "Test pattern did not fragment any lines" << std::endl;
    errors++;
  }

  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
