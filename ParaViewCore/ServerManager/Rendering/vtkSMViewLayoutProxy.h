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
//
// .SECTION Description
// vtkSMViewLayoutProxy is used by ParaView to layout multiple views in a 2D
// KD-Tree layout. This is proxy, hence can be registered with the proxy manager
// just like other regular proxies; participates in xml state saving/restoring,
// undo-redo, etc. Users can affects the GUI layout using this proxy instance
// from Python as well.
//
// Every time the vtkSMViewLayoutProxy changes so that it would affect the UI,
// this class fires vtkCommand::ConfigureEvent.
//
// View proxies that are to laid out in an layout should be "assigned" to a
// particular cell in a vtkSMViewLayoutProxy instance. vtkSMViewLayoutProxy
// takes over the responsibility of updating the view's \c Position property
// correctly. 
//
// Although, currently, there are no safe guards against assigning a view to
// more than one layout, this is strictly prohibited and can cause unexpected
// problems at run-time.

#ifndef __vtkSMViewLayoutProxy_h
#define __vtkSMViewLayoutProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkSMViewProxy;
class vtkImageData;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMViewLayoutProxy : public vtkSMProxy
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
  bool AssignView(int location, vtkSMViewProxy* view);

  // Description:
  // Similar to AssignView() except that is location specified is not available,
  // then this method treats the location merely as a hint and tries to find a
  // suitable place. First, if any empty cell is available, then that is used.
  // Second, if no empty cell is available and \c location is a valid cell, then
  // we either split the cell or traverse down the sub-tree from the cell and
  // split a cell to make room for the view. Thus, this method will always
  // assign the view to a frame. Returns the assigned location.
  int AssignViewToAnyCell(vtkSMViewProxy* view, int location_hint);

  // Description:
  // Removes a view. Returns the location of the cell emptied by the view, if
  // any, otherwise -1.
  int RemoveView(vtkSMViewProxy* view);
  bool RemoveView(int index);

  // Description:
  // Collapses a cell. Only leaf cells without any assigned views can be collapsed.
  // If the cell has a sibling, then that sibling is assigned to the parent
  // node and the sibling cell is destroyed as well. Returns true on success,
  // else false.
  bool Collapse(int location);

  // Description:
  // Swaps the cells at the two locations. Both locations must be leaf locations
  // i.e. cannot be split-cells.
  bool SwapCells(int location1, int location2);

  // Description:
  // Update the split fraction for a split cell. If IsSplitCell(location)
  // returns false, this method does not update the fraction.
  bool SetSplitFraction(int location, double fraction);

  // Description:
  // One can maximize a particular (non-split) cell. Note the maximized state is
  // restored as soon as the layout is changed or when RestoreMaximizedState()
  // is called. Returns false if the cell at the location cannot be maximized
  // since it's a split cell or invalid cell, true otherwise.
  bool MaximizeCell(int location);

  // Description:
  // Restores the maximized state.
  void RestoreMaximizedState();

  // Description:
  // Returns the maximized cell, if any. Returns -1 if no cell is currently
  // maximized.
  vtkGetMacro(MaximizedCell, int);

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
  vtkSMViewProxy* GetView(int location);

  // Description:
  // Returns the location for the view, of any. Returns -1 if the view is not
  // found.
  int GetViewLocation(vtkSMViewProxy*);

  // Description:
  // Updates positions for all views using the layout and current sizes.
  // This method is called automatically when the layout changes or the
  // "ViewSize" property on the assigned views changes.
  void UpdateViewPositions();

  // Description:
  // When in tile-display configuration, only 1 view-layout is shown on the
  // tile-display (for obvious reasons). To show any particular layout on the
  // tile display, simply call this method.
  void ShowViewsOnTileDisplay();

  // Description:
  // Captures an image from the layout (including all the views in the layout.
  vtkImageData* CaptureWindow(int magnification);

  // Description:
  // Overridden to save custom XML state.
  virtual vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root)
    { return this->Superclass::SaveXMLState(root); }
  virtual vtkPVXMLElement* SaveXMLState(
    vtkPVXMLElement* root, vtkSMPropertyIterator* iter);

  // Description:
  // Overridden to load custom XML state.
  virtual int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

  // Description:
  // Resets the layout.
  void Reset();

  // Description:
  // Helper method to locate a layout, if any that contains the specified view
  // proxy.
  static vtkSMViewLayoutProxy* FindLayout(vtkSMViewProxy*, const char* reggroup="layouts");

  // Description:
  // Set border size/color to use when capturing multiview images.
  static void SetMultiViewImageBorderColor(double r, double g, double b);
  static void SetMultiViewImageBorderWidth(int width);
  static const double* GetMultiViewImageBorderColor();
  static int GetMultiViewImageBorderWidth();

//BTX
protected:
  vtkSMViewLayoutProxy();
  ~vtkSMViewLayoutProxy();

  // Description:
  // Called to load state from protobuf message.
  virtual void LoadState(
    const vtkSMMessage* message, vtkSMProxyLocator* locator);

  // Description:
  // Although this class is a proxy, it's not really a proxy in the traditional
  // sense. So instead of using UpdateVTKObjects() to push state changes to the
  // server (or undo-redo stack), this new method is provided.
  virtual void UpdateState();

  // Description:
  // Starting with the cell-index, tries to find an empty cell in the sub-tree.
  // Returns -1 if none found. May return \c root, if root is indeed an empty
  // cell. Note this assumes that root is valid.
  int GetEmptyCell(int root);

  // Description:
  // Starting with the root, finds a splittable cell. Assumes \c root is valid.
  int GetSplittableCell(int root, Direction& suggested_direction);

  int MaximizedCell;
private:
  vtkSMViewLayoutProxy(const vtkSMViewLayoutProxy&); // Not implemented
  void operator=(const vtkSMViewLayoutProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;

  bool SetBlockUpdate(bool new_value)
    {
    bool temp = this->BlockUpdate;
    this->BlockUpdate = new_value;
    return temp;
    }

  bool BlockUpdate;

  static double MultiViewImageBorderColor[3];
  static int MultiViewImageBorderWidth;

//ETX
};

#endif
