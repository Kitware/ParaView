/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMViewLayoutProxy - vtkSMViewLayoutProxy is used by ParaView to layout
// multiple views in a 2D KD-Tree layout.
// .SECTION Description
// 

#ifndef __vtkSMViewLayoutProxy_h
#define __vtkSMViewLayoutProxy_h

#include "vtkSMProxy.h"

class vtkSMProxy;

class VTK_EXPORT vtkSMViewLayoutProxy : public vtkSMProxy
{
public:
  static vtkSMViewLayoutProxy* New();
  vtkTypeMacro(vtkSMViewLayoutProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum Direction
    {
    NONE,
    VERTICAL,
    HORIZONTAL
    };

  // Description:
  // Split a frame at the given \c location. Location must point to an existing cell
  // that's not split. If the location does not exist or is already split, then
  // returns 0, to indicate failure. Otherwise returns the index of the left (or
  // top) child node. The index for the sibling can be computed as (ret_val +
  // 1). \c fraction indicates a value in the range [0.0, 1.0] at which the cell
  // is split. If a View is set at the given location, it will be moved to the
  // left (or top) child after the split.
  unsigned int Split(unsigned int location, int direction, double fraction);
  unsigned int SplitVertical(unsigned int location, double fraction)
    { return this->Split(location, VERTICAL, fraction); }
  unsigned int SplitHorizontal(unsigned int location, double fraction)
    { return this->Split(location, HORIZONTAL, fraction); }

  // Description:
  // Assign a view at a particular location. Note that the view's position may
  // be changed by Split() calls. Returns true on success.
  bool AssignView(unsigned int location, vtkSMProxy* view);

  // Description:
  // Removes a view. Returns the location of the cell emptied by the view.
  unsigned int RemoveView(vtkSMProxy* view);

  // Description:
  // Collapses a cell. Only leaf cells without any assigned views can be collapsed.
  // If the cell has a sibling, then that sibling is assigned to the parent
  // node and the sibling cell is destroyed as well. Returns true on success,
  // else false.
  bool Collape(unsigned int location);

  // Description:
  // Update the split fraction for a split cell. If IsSplitCell(location)
  // returns false, this method does not update the fraction.
  bool SetSplitFraction(unsigned int location, double fraction);

  // Description:
  // Returns true if the cell identified by the location is a split cell.
  bool IsSplitCell(unsigned int location);

  // Description:
  // Returns the split direction for a split cell at the given location.
  Direction GetSplitDirection(unsigned int location);

  // Description:
  // Returns the split-fraction for a split cell at the given location.
  double GetSplitFraction(unsigned int location);

  // Description:
  // Returns the index for the first child of the given location. This does not
  // do any validity checks for the location, nor that of the child.
  unsigned int GetFirstChild(unsigned int location)
    { return 2*location + 1; }

  // Description:
  // Returns the index for the second child of the given location. This does not
  // do any validity checks for the location, nor that of the child.
  unsigned int GetSecondChild(unsigned int location)
    { return 2*location + 2; }

  // Description:
  // Returns the view, if any, assigned to the given cell location.
  vtkSMProxy* GetView(unsigned int location);

//BTX
protected:
  vtkSMViewLayoutProxy();
  ~vtkSMViewLayoutProxy();

private:
  vtkSMViewLayoutProxy(const vtkSMViewLayoutProxy&); // Not implemented
  void operator=(const vtkSMViewLayoutProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
