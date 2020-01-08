/*=========================================================================

  Program:   ParaView
  Module:    vtkPVHardwareSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVHardwareSelector
 * @brief   vtkHardwareSelector subclass with paraview
 * specific logic to avoid recapturing buffers unless needed.
 *
 * vtkHardwareSelector is subclass of vtkHardwareSelector that adds logic to
 * reuse the captured buffers as much as possible. Thus avoiding repeated
 * selection-rendering of repeated selections or picking.
 * This class does not know, however, when the cached buffers are invalid.
 * External logic must explicitly calls InvalidateCachedSelection() to ensure
 * that the cache is not reused.
*/

#ifndef vtkPVHardwareSelector_h
#define vtkPVHardwareSelector_h

#include "vtkOpenGLHardwareSelector.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkPVRenderView;

class VTKREMOTINGVIEWS_EXPORT vtkPVHardwareSelector : public vtkOpenGLHardwareSelector
{
public:
  static vtkPVHardwareSelector* New();
  vtkTypeMacro(vtkPVHardwareSelector, vtkOpenGLHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the view that will be used to exchange messages between all processes
   * involved. Note this does not affect the reference count of the view.
   */
  void SetView(vtkPVRenderView* view);

  /**
   * Overridden to avoid clearing of captured buffers.
   */
  vtkSelection* Select(int region[4]);

  /**
   * Same as Select() above, except this one use a polygon, instead
   * of a rectangle region, and select elements inside the polygon
   */
  vtkSelection* PolygonSelect(int* polygonPoints, vtkIdType count);

  /**
   * Returns true when the next call to Select() will result in renders to
   * capture the selection-buffers.
   */
  virtual bool NeedToRenderForSelection();

  /**
   * Called to invalidate the cache.
   */
  void InvalidateCachedSelection() { this->Modified(); }

  int AssignUniqueId(vtkProp*);

  // Fixes a -Woverloaded-virtual warning.
  using vtkOpenGLHardwareSelector::BeginRenderProp;
  /**
   * Set the local ProcessId.
   */
  void BeginRenderProp(vtkRenderWindow*) override;

protected:
  vtkPVHardwareSelector();
  ~vtkPVHardwareSelector() override;

  /**
   * Return a unique ID for the prop.
   */
  int GetPropID(int idx, vtkProp* prop) override;

  /**
   * Returns is the pass indicated is needed.
   * Overridden to report that we always need the process-id pass. In future, we
   * can be smart about it by only requiring it for sessions with more than 1
   * data-server.
   */
  bool PassRequired(int pass) override;

  /**
   * Prepare for selection.
   * Return false if CaptureBuffers() is false
   */
  bool PrepareSelect();

  void SavePixelBuffer(int passNo) override;

  vtkTimeStamp CaptureTime;
  int UniqueId;

private:
  vtkPVHardwareSelector(const vtkPVHardwareSelector&) = delete;
  void operator=(const vtkPVHardwareSelector&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
