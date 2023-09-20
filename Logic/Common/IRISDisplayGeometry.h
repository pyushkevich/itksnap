#ifndef IRISDISPLAYGEOMETRY_H
#define IRISDISPLAYGEOMETRY_H

#include "SNAPCommon.h"
#include <array>
#include <vnl/vnl_quaternion.h>

/**
 * \class IRISDisplayGeometry
 * \brief This structure encapsulates the coordinate mapping between
 * the three orthogonal slices in ITK-SNAP and the patient coordinate system.
 * It is characterized by three 3-character strings, which define how the
 * directions in the display space map to the directions in the anatomical
 * space.
 */
struct IRISDisplayGeometry
{
  /** Three strings of 3 characters each (RAILPS) defining the mapping */
  std::array<std::string, 3> DisplayToAnatomyRAI = { "RPS", "AIR", "RIP" };

  /** Allow for an additional freeform rotation around the cursor */
  vnl_quaternion<double> CustomRotation;

  /** Constructor: creates default mapping */
  IRISDisplayGeometry();

  /** Constructor taking three strings */
  IRISDisplayGeometry(const std::string &rai0,
                      const std::string &rai1,
                      const std::string &rai2);

  /** Get the display window corresponding to an anatomical direction */
  int GetDisplayWindowForAnatomicalDirection(AnatomicalDirection iAnat) const;

  /** Get the anatomical direction in the i-th display window */
  AnatomicalDirection GetAnatomicalDirectionForDisplayWindow(int iWin) const;
};

#endif // IRISDISPLAYGEOMETRY_H
