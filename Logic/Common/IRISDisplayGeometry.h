#ifndef IRISDISPLAYGEOMETRY_H
#define IRISDISPLAYGEOMETRY_H

#include "SNAPCommon.h"

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
  std::string DisplayToAnatomyRAI[3];

  /** Constructor: creates default mapping */
  IRISDisplayGeometry();

  /** Constructor taking three strings */
  IRISDisplayGeometry(const std::string &rai0,
                      const std::string &rai1,
                      const std::string &rai2);

  /** Copy constructor */
  IRISDisplayGeometry(const IRISDisplayGeometry &other);

  /** Assignment operator */
  IRISDisplayGeometry &operator = (const IRISDisplayGeometry &other);

  /** Get the display window corresponding to an anatomical direction */
  int GetDisplayWindowForAnatomicalDirection(AnatomicalDirection iAnat) const;

  /** Get the anatomical direction in the i-th display window */
  AnatomicalDirection GetAnatomicalDirectionForDisplayWindow(int iWin) const;

};

#endif // IRISDISPLAYGEOMETRY_H
