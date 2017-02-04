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
/**
 * @class   vtkPVCacheKeeperPipeline
 *
 *
*/

#ifndef vtkPVCacheKeeperPipeline_h
#define vtkPVCacheKeeperPipeline_h

#include "vtkCompositeDataPipeline.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class vtkPVCacheKeeper;
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVCacheKeeperPipeline
  : public vtkCompositeDataPipeline
{
public:
  static vtkPVCacheKeeperPipeline* New();
  vtkTypeMacro(vtkPVCacheKeeperPipeline, vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPVCacheKeeperPipeline();
  ~vtkPVCacheKeeperPipeline();

  virtual int ForwardUpstream(int i, int j, vtkInformation* request) VTK_OVERRIDE;
  virtual int ForwardUpstream(vtkInformation* request) VTK_OVERRIDE;

private:
  vtkPVCacheKeeperPipeline(const vtkPVCacheKeeperPipeline&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVCacheKeeperPipeline&) VTK_DELETE_FUNCTION;
};

#endif
