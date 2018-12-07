/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCoreUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCoreUtilities
 * @brief   collection of utilities.
 *
 * vtkSMCoreUtilities provides miscellaneous utility functions.
*/

#ifndef vtkSMCoreUtilities_h
#define vtkSMCoreUtilities_h

#include "vtkObject.h"
#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkStdString.h"                 // needed for vtkStdString.

class vtkSMProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMCoreUtilities : public vtkObject
{
public:
  static vtkSMCoreUtilities* New();
  vtkTypeMacro(vtkSMCoreUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Given a proxy (or proxy prototype), returns the name of the property that
   * ParaView application will be use as the default FileName property.
   * Returns the name of the property or NULL when no such property is found.
   */
  static const char* GetFileNameProperty(vtkSMProxy*);

  /**
   * Sanitize a label/name to be remove spaces, delimiters etc.
   */
  static vtkStdString SanitizeName(const char*);

  //@{
  /**
   * Given a range, converts it to be a valid range to switch to log space. If
   * the range is changed, returns true, otherwise returns false.
   */
  static bool AdjustRangeForLog(double range[2]);
  static bool AdjustRangeForLog(double& rmin, double& rmax)
  {
    double range[2] = { rmin, rmax };
    bool retVal = vtkSMCoreUtilities::AdjustRangeForLog(range);
    rmin = range[0];
    rmax = range[1];
    return retVal;
  }
  //@}

  //@{
  /**
   * Adjust the given range to make it suitable for use with color maps. The
   * current logic (which may change in future) does the following:
   * 1. If the range is invalid i.e range[1] < range[0], simply returns `false`
   *    and keeps the range unchanged.
   * 2. If the range[0] == range[1] (using logic to handle nearly similar
   *    floating points numbers), then the range[1] is adjusted to be such that
   *    range[1] > range[0p].
   * 3. If range[0] < range[1] (beyond the margin of error checked for in (2),
   *    then range is left unchanged.
   *
   * @returns `true` if the range was changed, `false` is the range was left
   * unchanged.
   */
  static bool AdjustRange(double range[2]);
  static bool AdjustRange(double& rmin, double& rmax)
  {
    double range[2] = { rmin, rmax };
    bool retVal = vtkSMCoreUtilities::AdjustRange(range);
    rmin = range[0];
    rmax = range[1];
    return retVal;
  }
  //@}

  //@{
  /**
   * Compares \c val1 and \c val2 and returns true is the two numbers are within
   * \c ulpsDiff ULPs (units in last place) from each other.
   */
  static bool AlmostEqual(const double range[2], int ulpsDiff);
  static bool AlmostEqual(double rmin, double rmax, int ulpsDiff)
  {
    double range[2] = { rmin, rmax };
    return vtkSMCoreUtilities::AlmostEqual(range, ulpsDiff);
  }
  //@}

  //@{
  /**
   * Given a proxy and a port number get the name of the input.
   */
  static const char* GetInputPropertyName(vtkSMProxy* proxy, int port = 0);
  //@}

  /**
   * Given a VTK cell type value from the enum in vtkCellTypes.h,
   * returns a string describing that cell type for use if ParaView's GUI.
   * For example it pasesd VTK_TRIANGLE it will return "Triangle".
   * If an unknown cell type is passed to this it returns the string "Unknown".
   */
  static const char* GetStringForCellType(int cellType);

protected:
  vtkSMCoreUtilities();
  ~vtkSMCoreUtilities() override;

private:
  vtkSMCoreUtilities(const vtkSMCoreUtilities&) = delete;
  void operator=(const vtkSMCoreUtilities&) = delete;
};

#endif
