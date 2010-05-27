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
// .NAME vtkIceTRenderer2
// .SECTION Description
//

#ifndef __vtkIceTRenderer2_h
#define __vtkIceTRenderer2_h

#include "vtkOpenGLRenderer.h"
#include "vtkIceTCompositePass.h"

class vtkPKdTree;

class VTK_EXPORT vtkIceTRenderer2 : public vtkOpenGLRenderer
{
public:
  static vtkIceTRenderer2* New();
  vtkTypeMacro(vtkIceTRenderer2, vtkOpenGLRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // *** Methods reimplemented from superclass ***

  // Description:
  // Concrete open gl render method.
  virtual void DeviceRender(void);

public:
  // *** Methods for configuring IceT ***

  // Description:
  // Set the tile dimensions. Default is (1, 1).
  // If any of the dimensions is > 1 then tile display mode is assumed.
  void SetTileDimensions(int x, int y)
    { this->IceTCompositePass->SetTileDimensions(x, y); }

  // Description:
  // Set the tile mullions. The mullions are measured in pixels. Use
  // negative numbers for overlap.
  void SetTileMullions(int x, int y)
    { this->IceTCompositePass->SetTileMullions(x, y); }

  // Description:
  // Set to true if data is replicated on all processes. This will enable IceT
  // to minimize communications since data is available on all process. Off by
  // default.
  void SetDataReplicatedOnAllProcesses(bool val)
    { this->IceTCompositePass->SetDataReplicatedOnAllProcesses(val); }

  // Description:
  // kd tree that gives processes ordering. Initial value is a NULL pointer.
  // This is used only when UseOrderedCompositing is true.
  void SetKdTree(vtkPKdTree *kdtree)
    { this->IceTCompositePass->SetKdTree(kdtree); }

  // Description:
  // Set this to true, if compositing must be done in a specific order. This is
  // necessary when rendering volumes or translucent geometries. When
  // UseOrderedCompositing is set to true, it is expected that the KdTree is set as
  // well. The KdTree is used to decide the process-order for compositing.
  void SetUseOrderedCompositing(bool uoc)
    { this->IceTCompositePass->SetUseOrderedCompositing(uoc); }

  // Description:
  // Set the image reduction factor. Overrides superclass implementation.
  virtual void SetImageReductionFactor(int val)
    { this->IceTCompositePass->SetImageReductionFactor(val); }
  virtual int GetImageReductionFactor()
    { return this->IceTCompositePass->GetImageReductionFactor(); }

//BTX
  void Draw();
protected:
  vtkIceTRenderer2();
  ~vtkIceTRenderer2();

  void IceTDeviceRender();

  vtkIceTCompositePass* IceTCompositePass;

private:
  vtkIceTRenderer2(const vtkIceTRenderer2&); // Not implemented
  void operator=(const vtkIceTRenderer2&); // Not implemented
//ETX
};

#endif
