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

#include "vtkHardwareSelector.h"

class VTK_EXPORT vtkPVHardwareSelector : public vtkHardwareSelector
{
public:
  static vtkPVHardwareSelector* New();
  vtkTypeMacro(vtkPVHardwareSelector, vtkHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to avoid clearing of captured buffers.
  vtkSelection* Select(int region[4]);

  // Description:
  // Returns true when the next call to Select() will result in renders to
  // capture the selection-buffers.
  virtual bool NeedToRenderForSelection();

  // Description:
  // Called to invalidate the cache.
  void InvalidateCachedSelection()
    { this->Modified(); }
//BTX
protected:
  vtkPVHardwareSelector();
  ~vtkPVHardwareSelector();

  vtkTimeStamp CaptureTime;
private:
  vtkPVHardwareSelector(const vtkPVHardwareSelector&); // Not implemented
  void operator=(const vtkPVHardwareSelector&); // Not implemented
//ETX
};

#endif
