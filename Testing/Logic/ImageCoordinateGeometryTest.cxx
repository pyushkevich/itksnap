#include "ImageCoordinateGeometry.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <string>

namespace
{
using DirectionMatrix = ImageCoordinateGeometry::DirectionMatrix;

bool ExpectCode(const std::string &name,
                const DirectionMatrix &matrix,
                const std::string &expected)
{
  const std::string actual =
    ImageCoordinateGeometry::ConvertDirectionMatrixToClosestRAICode(matrix);

  if(actual != expected)
    {
    std::cerr << name << ": expected " << expected << ", got " << actual
              << std::endl;
    return false;
    }

  if(!ImageCoordinateGeometry::IsRAICodeValid(actual))
    {
    std::cerr << name << ": result is not a valid RAI code: " << actual
              << std::endl;
    return false;
    }

  return true;
}
}

int main()
{
  bool success = true;

  // Every signed cardinal orientation must round-trip exactly. This covers
  // all 3! axis permutations and all 2^3 sign combinations (48 codes).
  const std::string positive_letters("RAI");
  const std::string negative_letters("LPS");
  std::array<size_t, 3> permutation = {{0, 1, 2}};
  do
    {
    for(unsigned int sign_bits = 0; sign_bits < 8; ++sign_bits)
      {
      std::string code("...");
      for(size_t column = 0; column < 3; ++column)
        {
        const size_t row = permutation[column];
        const bool positive = (sign_bits & (1u << column)) != 0;
        code[column] = positive ? positive_letters[row] : negative_letters[row];
        }

      const DirectionMatrix cardinal =
        ImageCoordinateGeometry::ConvertRAICodeToDirectionMatrix(code);
      success = ExpectCode("cardinal " + code, cardinal, code) && success;
      }
    }
  while(std::next_permutation(permutation.begin(), permutation.end()));

  // Real oblique direction matrix that the previous per-column argmax
  // converted to the invalid code SAP. The closest one-to-one assignment is
  // SAR.
  DirectionMatrix sap(3, 3);
  sap(0, 0) = -0.562583101; sap(0, 1) =  0.530269256; sap(0, 2) =  0.634282828;
  sap(1, 0) =  0.000000000; sap(1, 1) =  0.767208939; sap(1, 2) = -0.641397585;
  sap(2, 0) = -0.826740742; sap(2, 1) = -0.360839245; sap(2, 2) = -0.431618387;
  success = ExpectCode("oblique SAP regression", sap, "SAR") && success;

  // Real oblique direction matrix that the previous implementation converted
  // to SAA. A global assignment uses each anatomical axis exactly once and
  // selects SRA.
  DirectionMatrix saa(3, 3);
  saa(0, 0) = -0.616757270; saa(0, 1) =  0.558802430; saa(0, 2) = -0.554391840;
  saa(1, 0) =  0.000000010; saa(1, 1) =  0.704299630; saa(1, 2) =  0.709902840;
  saa(2, 0) = -0.787153390; saa(2, 1) = -0.437837730; saa(2, 2) =  0.434381920;
  success = ExpectCode("oblique SAA regression", saa, "SRA") && success;

  // The original NIfTI affine from GitHub issue #69 is expressed in RAS
  // coordinates. ITK converts NIfTI geometry to LPS by negating the first two
  // rows before this function sees the direction matrix. The previous
  // per-column argmax produced IRI; the global assignment recovers RAI.
  DirectionMatrix issue_69_ras(3, 3);
  issue_69_ras(0, 0) = -0.656692; issue_69_ras(0, 1) = -0.676962; issue_69_ras(0, 2) =  0.332383;
  issue_69_ras(1, 0) =  0.360389; issue_69_ras(1, 1) = -0.668844; issue_69_ras(1, 2) = -0.650205;
  issue_69_ras(2, 0) =  0.662477; issue_69_ras(2, 1) = -0.307197; issue_69_ras(2, 2) =  0.683194;

  DirectionMatrix issue_69_lps = issue_69_ras;
  for(size_t column = 0; column < 3; ++column)
    {
    issue_69_lps(0, column) *= -1.0;
    issue_69_lps(1, column) *= -1.0;
    }
  success = ExpectCode("issue 69 IRI regression", issue_69_lps, "RAI") && success;

  // A clinical shoulder MRI direction matrix posted in a later comment on
  // issue #69. Independent column selection maps two columns to the left-right
  // axis; the global assignment produces the valid closest code LSP.
  DirectionMatrix shoulder(3, 3);
  shoulder(0, 0) = -0.71844119; shoulder(0, 1) =  0.06575722; shoulder(0, 2) = -0.69247257;
  shoulder(1, 0) =  0.67544218; shoulder(1, 1) =  0.30380508; shoulder(1, 2) = -0.67192283;
  shoulder(2, 0) =  0.16619304; shoulder(2, 1) = -0.95046224; shoulder(2, 2) = -0.26268128;
  success = ExpectCode("issue 69 shoulder MRI regression",
                       shoulder, "LSP") && success;

  return success ? 0 : 1;
}
