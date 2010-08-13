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
// .NAME vtkPVSynchronizedRenderer - synchronizes and composites renderers among
// processes in ParaView configurations.
// .SECTION Description
//

#ifndef __vtkPVSynchronizedRenderer_h
#define __vtkPVSynchronizedRenderer_h

#include "vtkObject.h"

class vtkSynchronizedRenderers;
class vtkIceTSynchronizedRenderers;
class vtkRenderer;

class VTK_EXPORT vtkPVSynchronizedRenderer : public vtkObject
{
public:
  static vtkPVSynchronizedRenderer* New();
  vtkTypeMacro(vtkPVSynchronizedRenderer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the renderer that is being synchronized.
  void SetRenderer(vtkRenderer*);

  // Description:
  // Enable/Disable parallel rendering.
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);

  // Description:
  // Get/Set the image reduction factor.
  // This needs to be set on all processes and must match up.
  void SetImageReductionFactor(int);
  vtkGetMacro(ImageReductionFactor, int);

  // Description:
  // Compute visible props bounds. This method must be called on all processes.
  // It will result is providing the full data bounds on all processes involved.
  // NOTE: If this method is not called on all processes at the same time, it
  // WILL result in deadlocks.
  void ComputeVisiblePropBounds(double bounds[6]);

//BTX
protected:
  vtkPVSynchronizedRenderer();
  ~vtkPVSynchronizedRenderer();

  vtkSynchronizedRenderers* CSSynchronizer;
  vtkSynchronizedRenderers* ParallelSynchronizer;

  enum ModeEnum
    {
    INVALID,
    BUILTIN,
    CLIENT,
    SERVER,
    BATCH
    };

  ModeEnum Mode;
  bool Enabled;
  int ImageReductionFactor;
  vtkRenderer* Renderer;
private:
  vtkPVSynchronizedRenderer(const vtkPVSynchronizedRenderer&); // Not implemented
  void operator=(const vtkPVSynchronizedRenderer&); // Not implemented
//ETX
};

#endif
