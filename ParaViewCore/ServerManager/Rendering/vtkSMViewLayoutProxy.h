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
/**
 * @class   vtkSMViewLayoutProxy
 * @brief   vtkSMViewLayoutProxy is used by ParaView to layout
 * multiple views in a 2D KD-Tree layout.
 *
 *
 * vtkSMViewLayoutProxy is used by ParaView to layout multiple views in a 2D
 * KD-Tree layout. This is proxy, hence can be registered with the proxy manager
 * just like other regular proxies; participates in xml state saving/restoring,
 * undo-redo, etc. Users can affects the GUI layout using this proxy instance
 * from Python as well.
 *
 * Every time the vtkSMViewLayoutProxy changes so that it would affect the UI,
 * this class fires vtkCommand::ConfigureEvent.
 *
 * View proxies that are to laid out in an layout should be "assigned" to a
 * particular cell in a vtkSMViewLayoutProxy instance. vtkSMViewLayoutProxy
 * takes over the responsibility of updating the view's \c Position property
 * correctly.
 *
 * Although, currently, there are no safe guards against assigning a view to
 * more than one layout, this is strictly prohibited and can cause unexpected
 * problems at run-time.
*/

#ifndef vtkSMViewLayoutProxy_h
#define vtkSMViewLayoutProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMProxy.h"
#include <vector> // needed for std::vector.

class vtkSMViewProxy;
class vtkImageData;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMViewLayoutProxy : public vtkSMProxy
{
public:
  static vtkSMViewLayoutProxy* New();
  vtkTypeMacro(vtkSMViewLayoutProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  enum Direction
  {
    NONE,
    VERTICAL,
    HORIZONTAL
  };

  /**
   * Split a frame at the given \c location. Location must point to an existing cell
   * that's not split. If the location does not exist or is already split, then
   * returns -1, to indicate failure. Otherwise returns the index of the left (or
   * top) child node. The index for the sibling can be computed as (ret_val +
   * 1). \c fraction indicates a value in the range [0.0, 1.0] at which the cell
   * is split. If a View is set at the given location, it will be moved to the
   * left (or top) child after the split.
   */
  int Split(int location, int direction, double fraction);
  int SplitVertical(int location, double fraction)
  {
    return this->Split(location, VERTICAL, fraction);
  }
  int SplitHorizontal(int location, double fraction)
  {
    return this->Split(location, HORIZONTAL, fraction);
  }

  /**
   * Assign a view at a particular location. Note that the view's position may
   * be changed by Split() calls. Returns true on success.
   */
  bool AssignView(int location, vtkSMViewProxy* view);

  /**
   * Similar to AssignView() except that is location specified is not available,
   * then this method treats the location merely as a hint and tries to find a
   * suitable place. First, if any empty cell is available, then that is used.
   * Second, if no empty cell is available and \c location is a valid cell, then
   * we either split the cell or traverse down the sub-tree from the cell and
   * split a cell to make room for the view. Thus, this method will always
   * assign the view to a frame. Returns the assigned location.
   */
  int AssignViewToAnyCell(vtkSMViewProxy* view, int location_hint);

  //@{
  /**
   * Removes a view. Returns the location of the cell emptied by the view, if
   * any, otherwise -1.
   */
  int RemoveView(vtkSMViewProxy* view);
  bool RemoveView(int index);
  //@}

  /**
   * Collapses a cell. Only leaf cells without any assigned views can be collapsed.
   * If the cell has a sibling, then that sibling is assigned to the parent
   * node and the sibling cell is destroyed as well. Returns true on success,
   * else false.
   */
  bool Collapse(int location);

  /**
   * Swaps the cells at the two locations. Both locations must be leaf locations
   * i.e. cannot be split-cells.
   */
  bool SwapCells(int location1, int location2);

  /**
   * Update the split fraction for a split cell. If IsSplitCell(location)
   * returns false, this method does not update the fraction.
   */
  bool SetSplitFraction(int location, double fraction);

  /**
   * One can maximize a particular (non-split) cell. Note the maximized state is
   * restored as soon as the layout is changed or when RestoreMaximizedState()
   * is called. Returns false if the cell at the location cannot be maximized
   * since it's a split cell or invalid cell, true otherwise.
   */
  bool MaximizeCell(int location);

  /**
   * Restores the maximized state.
   */
  void RestoreMaximizedState();

  //@{
  /**
   * Returns the maximized cell, if any. Returns -1 if no cell is currently
   * maximized.
   */
  vtkGetMacro(MaximizedCell, int);
  //@}

  /**
   * Returns true if the cell identified by the location is a split cell.
   */
  bool IsSplitCell(int location);

  /**
   * Returns the split direction for a split cell at the given location.
   */
  Direction GetSplitDirection(int location);

  /**
   * Returns the split-fraction for a split cell at the given location.
   */
  double GetSplitFraction(int location);

  /**
   * Returns the index for the first child of the given location. This does not
   * do any validity checks for the location, nor that of the child.
   */
  static int GetFirstChild(int location) { return 2 * location + 1; }

  /**
   * Returns the index for the second child of the given location. This does not
   * do any validity checks for the location, nor that of the child.
   */
  static int GetSecondChild(int location) { return 2 * location + 2; }

  /**
   * Returns the parent index.
   */
  static int GetParent(int location) { return (location > 0 ? ((location - 1) / 2) : -1); }

  /**
   * Returns the view, if any, assigned to the given cell location.
   */
  vtkSMViewProxy* GetView(int location);

  /**
   * Returns the location for the view, of any. Returns -1 if the view is not
   * found.
   */
  int GetViewLocation(vtkSMViewProxy*);

  //@{
  /**
   * Returns if a view is contained in this layout.
   */
  bool ContainsView(vtkSMViewProxy* view) { return this->GetViewLocation(view) != -1; }
  bool ContainsView(vtkSMProxy* view);
  //@}

  /**
   * Updates positions for all views using the layout and current sizes.
   * This method is called automatically when the layout changes or the
   * "ViewSize" property on the assigned views changes.
   */
  void UpdateViewPositions();

  /**
   * When in tile-display configuration, only 1 view-layout is shown on the
   * tile-display (for obvious reasons). To show any particular layout on the
   * tile display, simply call this method.
   */
  void ShowViewsOnTileDisplay();

  /**
   * Captures an image from the layout (including all the views in the layout.
   */
  vtkImageData* CaptureWindow(int magnification);

  /**
   * Overridden to save custom XML state.
   */
  virtual vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root) VTK_OVERRIDE
  {
    return this->Superclass::SaveXMLState(root);
  }
  virtual vtkPVXMLElement* SaveXMLState(
    vtkPVXMLElement* root, vtkSMPropertyIterator* iter) VTK_OVERRIDE;

  /**
   * Overridden to load custom XML state.
   */
  virtual int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  /**
   * Resets the layout.
   */
  void Reset();

  /**
   * Returns the extents for all views in the layout.
   */
  void GetLayoutExtent(int extent[4]);

  /**
   * Update the size for all the views in the layout assuming the new size
   * provided for the whole layout.
   */
  void SetSize(const int size[2]);

  /**
   * Helper method to locate a layout, if any that contains the specified view
   * proxy.
   */
  static vtkSMViewLayoutProxy* FindLayout(vtkSMViewProxy*, const char* reggroup = "layouts");

  /**
   * Returns a vector of the view proxies added to his layout.
   */
  std::vector<vtkSMViewProxy*> GetViews();

  /**
   * Set the color to use for separator between views in multi-view
   * configurations when saving images.
   * @param[in] r Red component value in range (0, 255);
   * @param[in] g Green component value in range (0, 255);
   * @param[in] b Blue component value in range (0, 255);
   */
  void SetSeparatorColor(unsigned char r, unsigned char g, unsigned char b);

  //@{
  /**
   * Set the color to use for separator between views in multi-view
   * configurations when saving images.
   *
   * The arguments are the components of the red, green, and blue channels from 0.0 to 1.0.
   */
  vtkSetVector3Macro(SeparatorColor, double);
  vtkGetVector3Macro(SeparatorColor, double);
  //@}

  //@{
  /**
   * Get/Set the separator width (in pixels) to use for separator between views
   * in multi-view configurations.
   */
  vtkSetClampMacro(SeparatorWidth, int, 0, VTK_INT_MAX);
  vtkGetMacro(SeparatorWidth, int);
  //@}

protected:
  vtkSMViewLayoutProxy();
  ~vtkSMViewLayoutProxy();

  /**
   * Called to load state from protobuf message.
   */
  virtual void LoadState(const vtkSMMessage* message, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  /**
   * Although this class is a proxy, it's not really a proxy in the traditional
   * sense. So instead of using UpdateVTKObjects() to push state changes to the
   * server (or undo-redo stack), this new method is provided.
   */
  virtual void UpdateState();

  /**
   * Starting with the cell-index, tries to find an empty cell in the sub-tree.
   * Returns -1 if none found. May return \c root, if root is indeed an empty
   * cell. Note this assumes that root is valid.
   */
  int GetEmptyCell(int root);

  /**
   * Starting with the root, finds a splittable cell. Assumes \c root is valid.
   */
  int GetSplittableCell(int root, Direction& suggested_direction);

  int MaximizedCell;

  double SeparatorColor[3];
  int SeparatorWidth;

private:
  vtkSMViewLayoutProxy(const vtkSMViewLayoutProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMViewLayoutProxy&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;

  bool SetBlockUpdate(bool new_value)
  {
    bool temp = this->BlockUpdate;
    this->BlockUpdate = new_value;
    return temp;
  }

  bool BlockUpdate;

  bool SetBlockUpdateViewPositions(bool val)
  {
    bool temp = this->BlockUpdateViewPositions;
    this->BlockUpdateViewPositions = val;
    return temp;
  }
  bool BlockUpdateViewPositions;
};

#endif
