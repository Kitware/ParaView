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
// .NAME vtkPVHardwareSelector - vtkHardwareSelector subclass with paraview
// sepecific logic to avoid recapturing buffers unless needed.
// .SECTION Description
// vtkHardwareSelector is subclass of vtkHardwareSelector that adds logic to
// reuse the captured buffers as much as possible. Thus avoiding repeated
// selection-rendering of repeated selections or picking.
// This class does not know, however, when the cached buffers are invalid.
// External logic must explicitly calls InvalidateCachedSelection() to ensure
// that the cache is not reused.

#ifndef __vtkPVHardwareSelector_h
#define __vtkPVHardwareSelector_h

#include "vtkOpenGLHardwareSelector.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkPVSynchronizedRenderWindows;
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVHardwareSelector : public vtkOpenGLHardwareSelector
{
public:
  static vtkPVHardwareSelector* New();
  vtkTypeMacro(vtkPVHardwareSelector, vtkOpenGLHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to avoid clearing of captured buffers.
  vtkSelection* Select(int region[4]);

  // Description:
  // Same as Select() above, except this one use a polygon, instead
  // of a rectangle region, and select elements inside the polygon
  vtkSelection* PolygonSelect(int* polygonPoints, vtkIdType count);

  // Description:
  // Returns true when the next call to Select() will result in renders to
  // capture the selection-buffers.
  virtual bool NeedToRenderForSelection();

  // Description:
  // Called to invalidate the cache.
  void InvalidateCachedSelection()
    { this->Modified(); }

  int AssignUniqueId(vtkProp*);

  // Description:
  // Set the vtkPVSynchronizedRenderWindows instance. This is used to
  // communicate between all active processes.
  void SetSynchronizedWindows(vtkPVSynchronizedRenderWindows*);

//BTX
protected:
  vtkPVHardwareSelector();
  ~vtkPVHardwareSelector();

  // Description:
  // Return a unique ID for the prop.
  virtual int GetPropID(int idx, vtkProp* prop);

  // Description:
  // Returns is the pass indicated is needed.
  // Overridden to report that we always need the process-id pass. In future, we
  // can be smart about it by only requiring it for sessions with more than 1
  // data-server.
  virtual bool PassRequired(int pass);

  // Description:
  // Prepare for selection.
  // Return false if CaptureBuffers() is false
  bool PrepareSelect();

  vtkTimeStamp CaptureTime;
  int UniqueId;
  vtkWeakPointer<vtkPVSynchronizedRenderWindows> SynchronizedWindows;
private:
  vtkPVHardwareSelector(const vtkPVHardwareSelector&); // Not implemented
  void operator=(const vtkPVHardwareSelector&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
