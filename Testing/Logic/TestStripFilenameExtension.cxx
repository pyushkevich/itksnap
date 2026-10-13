// Test: StripFilenameExtension, which gives layers their default nicknames.
//
// Regression test: the nickname was cut at the first period of the filename,
// so "T1_0.8x0.8x0.8.nii.gz" became "T1_0".

#include "SNAPCommon.h"
#include <iostream>
#include <string>

static int n_failures = 0;

static void Check(const std::string &filename, const std::string &expected)
{
  std::string result = StripFilenameExtension(filename);
  if (result != expected)
  {
    std::cerr << "FAILED: \"" << filename << "\" -> \"" << result << "\", expected \""
              << expected << "\"" << std::endl;
    ++n_failures;
  }
}

int main(int, char *[])
{
  // Periods in the name are kept
  Check("T1_3D_0.8x0.8x0.8_20260914112029_3.nii.gz", "T1_3D_0.8x0.8x0.8_20260914112029_3");
  Check("/data/subj.v2/T1_0.8mm.nii", "T1_0.8mm");
  Check("brain.v2.nrrd", "brain.v2");

  // Plain and compound extensions
  Check("image.nii.gz", "image");
  Check("image.NII.GZ", "image");
  Check("image.img.gz", "image");
  Check("image.hdr", "image");
  Check("image.mha", "image");
  Check("image.vti", "image");
  Check("project.itksnap", "project");
  Check("mesh.vtk", "mesh");
  Check("labels.txt", "labels");
  Check("sequence.seq.nrrd", "sequence");
  Check("sequence.seq.nhdr", "sequence");
  Check("image.tar.bz2", "image");

  // No extension, or something that only looks like one
  Check("T1_0.8mm", "T1_0.8mm");
  Check("T1_0.8x0", "T1_0.8x0");
  Check("dicom_dir", "dicom_dir");
  Check(".hidden", ".hidden");
  Check("image.toolongextension", "image.toolongextension");
  Check("x.gz", "x");
  Check("seq.nrrd", "seq");

  if (n_failures)
    std::cerr << n_failures << " check(s) failed" << std::endl;
  else
    std::cout << "All checks passed" << std::endl;
  return n_failures ? 1 : 0;
}
