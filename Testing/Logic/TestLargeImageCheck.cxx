/**
 * TestLargeImageCheck.cxx
 *
 * Verifies that GuidedNativeImageIO throws a clean IRISException when asked to
 * load an image whose declared size exceeds available system memory, instead of
 * crashing with SIGBUS.
 *
 * The test writes a minimal 352-byte NIfTI-1 header that declares a
 * 2000 x 2000 x 2000 float32 volume (~32 GB uncompressed). The size guard in
 * ReadNativeImageData() must fire before image->Allocate() is called, so the
 * actual file on disk doesn't need to contain any image data.
 */

#include "GuidedNativeImageIO.h"
#include "IRISException.h"
#include "Registry.h"

#include "itksys/SystemTools.hxx"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Write a field into a byte buffer at a given byte offset. */
template<typename T>
static void WriteField(uint8_t *buf, size_t offset, T value)
{
  std::memcpy(buf + offset, &value, sizeof(T));
}

/**
 * Write a minimal valid NIfTI-1 (.nii) header for a 3-D float32 volume of
 * the requested dimensions. The header is exactly 348 bytes, followed by a
 * 4-byte extension block (all zeros → no extensions), for a total of 352 bytes.
 *
 * NIfTI-1 spec: https://nifti.nimh.nih.gov/nifti1/documentation
 */
static void WriteLargeNiftiHeader(const std::string &path,
                                  int16_t nx, int16_t ny, int16_t nz)
{
  uint8_t hdr[348];
  std::memset(hdr, 0, sizeof(hdr));

  // sizeof_hdr = 348  (offset 0, int32)
  WriteField<int32_t>(hdr, 0, 348);

  // dim[0..3]: ndims=3, then nx, ny, nz  (offset 40, 8 x int16)
  WriteField<int16_t>(hdr, 40, 3);
  WriteField<int16_t>(hdr, 42, nx);
  WriteField<int16_t>(hdr, 44, ny);
  WriteField<int16_t>(hdr, 46, nz);
  WriteField<int16_t>(hdr, 48, 1);   // dim[4]
  WriteField<int16_t>(hdr, 50, 1);   // dim[5]
  WriteField<int16_t>(hdr, 52, 1);   // dim[6]
  WriteField<int16_t>(hdr, 54, 1);   // dim[7]

  // datatype = 16 (DT_FLOAT32), bitpix = 32  (offsets 70, 72, int16)
  WriteField<int16_t>(hdr, 70, 16);
  WriteField<int16_t>(hdr, 72, 32);

  // pixdim[0..4] = 1.0 mm  (offset 76, 8 x float32)
  for(int i = 0; i < 8; i++)
    WriteField<float>(hdr, 76 + i*4, 1.0f);

  // vox_offset = 352.0 (header + extension block)  (offset 108, float32)
  WriteField<float>(hdr, 108, 352.0f);

  // scl_slope = 1.0  (offset 112, float32)
  WriteField<float>(hdr, 112, 1.0f);

  // xyzt_units = 2 (NIFTI_UNITS_MM)  (offset 123, int8)
  hdr[123] = 2;

  // qform_code = sform_code = 1  (offsets 252, 254, int16)
  WriteField<int16_t>(hdr, 252, 1);
  WriteField<int16_t>(hdr, 254, 1);

  // srow_x/y/z: identity 1 mm spacing  (offsets 280, 296, 312, 4 x float32)
  WriteField<float>(hdr, 280, 1.0f);  // srow_x[0]
  WriteField<float>(hdr, 308, 1.0f);  // srow_y[1]
  WriteField<float>(hdr, 324, 0.0f);  WriteField<float>(hdr, 328, 0.0f);
  WriteField<float>(hdr, 332, 1.0f);  // srow_z[2]

  // magic = 'n+1\0'  (offset 344, 4 bytes) → single .nii file
  hdr[344] = 'n'; hdr[345] = '+'; hdr[346] = '1'; hdr[347] = '\0';

  std::ofstream f(path, std::ios::binary);
  if(!f)
    throw std::runtime_error("Cannot open temp file for writing: " + path);
  f.write(reinterpret_cast<const char *>(hdr), sizeof(hdr));

  // 4-byte extension block: all zeros means no extensions
  const uint8_t ext[4] = {0, 0, 0, 0};
  f.write(reinterpret_cast<const char *>(ext), 4);
  // Note: no image data bytes needed — the size check fires before Allocate().
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

/**
 * TestOversizedImageThrows
 *
 * Creates a NIfTI header declaring 2000³ × float32 ≈ 32 GB.
 * Expects ReadNativeImageData() to throw IRISException before allocating.
 */
static bool TestOversizedImageThrows(const std::string &tempDir)
{
  std::cout << "TestOversizedImageThrows ... " << std::flush;

  std::string tmpFile = tempDir + "/oversized_test.nii";

  // 2000^3 * 4 bytes = 32,000,000,000 bytes (~32 GB) — exceeds any machine's
  // available RAM, so the guard must always trigger.
  WriteLargeNiftiHeader(tmpFile, 2000, 2000, 2000);

  Registry hints;
  GuidedNativeImageIO::Pointer io = GuidedNativeImageIO::New();

  bool caughtExpected = false;
  try
    {
    io->ReadNativeImageHeader(tmpFile.c_str(), hints);
    io->ReadNativeImageData();
    }
  catch(IRISException &ex)
    {
    std::string msg = ex.what();
    if(msg.find("too large") != std::string::npos ||
       msg.find("available") != std::string::npos)
      {
      caughtExpected = true;
      std::cout << "PASS  (caught: " << msg << ")" << std::endl;
      }
    else
      {
      std::cout << "FAIL  (unexpected IRISException: " << msg << ")" << std::endl;
      }
    }
  catch(std::exception &ex)
    {
    std::cout << "FAIL  (unexpected exception: " << ex.what() << ")" << std::endl;
    }
  catch(...)
    {
    std::cout << "FAIL  (unknown exception)" << std::endl;
    }

  itksys::SystemTools::RemoveFile(tmpFile);
  return caughtExpected;
}

/**
 * TestNormalSizedImageNoThrow
 *
 * Creates a NIfTI header declaring a small 64³ float32 volume (1 MB).
 * ReadNativeImageData() should NOT throw the size-check exception.
 * (It may throw because the file has no pixel data, but not our size guard.)
 */
static bool TestNormalSizedImageNoThrow(const std::string &tempDir)
{
  std::cout << "TestNormalSizedImageNoThrow ... " << std::flush;

  std::string tmpFile = tempDir + "/small_test.nii";
  WriteLargeNiftiHeader(tmpFile, 64, 64, 64);  // 64^3 * 4 = 1 MB

  Registry hints;
  GuidedNativeImageIO::Pointer io = GuidedNativeImageIO::New();

  bool sizeGuardFired = false;
  try
    {
    io->ReadNativeImageHeader(tmpFile.c_str(), hints);
    io->ReadNativeImageData();
    }
  catch(IRISException &ex)
    {
    std::string msg = ex.what();
    if(msg.find("too large") != std::string::npos)
      {
      sizeGuardFired = true;
      std::cout << "FAIL  (size guard incorrectly fired for small image: " << msg << ")" << std::endl;
      }
    else
      {
      // Some other IRISException (e.g., truncated file) — acceptable
      std::cout << "PASS  (no size guard; other exception: " << msg << ")" << std::endl;
      }
    }
  catch(std::exception &ex)
    {
    // Expected: ITK may complain the file is too short — that's fine
    std::cout << "PASS  (no size guard; ITK error: " << ex.what() << ")" << std::endl;
    }
  catch(...)
    {
    std::cout << "PASS  (no size guard; unknown non-IRISException)" << std::endl;
    }

  itksys::SystemTools::RemoveFile(tmpFile);
  return !sizeGuardFired;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  // Use the directory of the test binary (or a supplied temp dir) for temp files
  std::string tempDir = itksys::SystemTools::GetFilenamePath(argv[0]);
  if(argc > 1)
    tempDir = argv[1];

  std::cout << "=== TestLargeImageCheck ===" << std::endl;
  std::cout << "Temp directory: " << tempDir << std::endl;

  int failed = 0;
  if(!TestOversizedImageThrows(tempDir))  failed++;
  if(!TestNormalSizedImageNoThrow(tempDir)) failed++;

  std::cout << std::endl;
  if(failed == 0)
    std::cout << "All tests PASSED." << std::endl;
  else
    std::cout << failed << " test(s) FAILED." << std::endl;

  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
