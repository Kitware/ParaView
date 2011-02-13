/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCacheKeeperPipeline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCacheKeeperPipeline
// .SECTION Description
//

#ifndef __vtkPVCacheKeeperPipeline_h
#define __vtkPVCacheKeeperPipeline_h

#include "vtkCompositeDataPipeline.h"

class vtkPVCacheKeeper;
class VTK_EXPORT vtkPVCacheKeeperPipeline : public vtkCompositeDataPipeline
{
public:
  static vtkPVCacheKeeperPipeline* New();
  vtkTypeMacro(vtkPVCacheKeeperPipeline, vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVCacheKeeperPipeline();
  ~vtkPVCacheKeeperPipeline();

  virtual int ForwardUpstream(int i, int j, vtkInformation* request);
  virtual int ForwardUpstream(vtkInformation* request);
private:
  vtkPVCacheKeeperPipeline(const vtkPVCacheKeeperPipeline&); // Not implemented
  void operator=(const vtkPVCacheKeeperPipeline&); // Not implemented
//ETX
};

#endif

