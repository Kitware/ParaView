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
  // returns -1, to indicate failure. Otherwise returns the index of the left (or
  // top) child node. The index for the sibling can be computed as (ret_val +
  // 1). \c fraction indicates a value in the range [0.0, 1.0] at which the cell
  // is split. If a View is set at the given location, it will be moved to the
  // left (or top) child after the split.
  int Split(int location, int direction, double fraction);
  int SplitVertical(int location, double fraction)
    { return this->Split(location, VERTICAL, fraction); }
  int SplitHorizontal(int location, double fraction)
    { return this->Split(location, HORIZONTAL, fraction); }

  // Description:
  // Assign a view at a particular location. Note that the view's position may
  // be changed by Split() calls. Returns true on success.
  bool AssignView(int location, vtkSMProxy* view);

  // Description:
  // Similar to AssignView() except that is location specified is not available,
  // then this method treats the location merely as a hint and tries to find a
  // suitable place. First, if any empty cell is available, then that is used.
  // Second, if no empty cell is available and \c location is a valid cell, then
  // we either split the cell or traverse down the sub-tree from the cell and
  // split a cell to make room for the view. Thus, this method will always
  // assign the view to a frame. Returns the assigned location.
  int AssignViewToAnyCell(vtkSMProxy* view, int location_hint);

  // Description:
  // Removes a view. Returns the location of the cell emptied by the view, if
  // any, otherwise -1.
  int RemoveView(vtkSMProxy* view);

  // Description:
  // Collapses a cell. Only leaf cells without any assigned views can be collapsed.
  // If the cell has a sibling, then that sibling is assigned to the parent
  // node and the sibling cell is destroyed as well. Returns true on success,
  // else false.
  bool Collape(int location);

  // Description:
  // Update the split fraction for a split cell. If IsSplitCell(location)
  // returns false, this method does not update the fraction.
  bool SetSplitFraction(int location, double fraction);

  // Description:
  // Returns true if the cell identified by the location is a split cell.
  bool IsSplitCell(int location);

  // Description:
  // Returns the split direction for a split cell at the given location.
  Direction GetSplitDirection(int location);

  // Description:
  // Returns the split-fraction for a split cell at the given location.
  double GetSplitFraction(int location);

  // Description:
  // Returns the index for the first child of the given location. This does not
  // do any validity checks for the location, nor that of the child.
  static int GetFirstChild(int location)
    { return 2*location + 1; }

  // Description:
  // Returns the index for the second child of the given location. This does not
  // do any validity checks for the location, nor that of the child.
  static int GetSecondChild(int location)
    { return 2*location + 2; }

  // Description:
  // Returns the parent index.
  static int GetParent(int location)
    { return (location > 0?  ( (location - 1) / 2 ) : -1); }

  // Description:
  // Returns the view, if any, assigned to the given cell location.
  vtkSMProxy* GetView(int location);

//BTX
protected:
  vtkSMViewLayoutProxy();
  ~vtkSMViewLayoutProxy();

  // Description:
  // Starting with the cell-index, tries to find an empty cell in the sub-tree.
  // Returns -1 if none found. May return \c root, if root is indeed an empty
  // cell. Note this assumes that root is valid.
  int GetEmptyCell(int root);

  // Description:
  // Starting with the root, finds a splittable cell. Assumes \c root is valid.
  int GetSplittableCell(int root, Direction& suggested_direction);

private:
  vtkSMViewLayoutProxy(const vtkSMViewLayoutProxy&); // Not implemented
  void operator=(const vtkSMViewLayoutProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
