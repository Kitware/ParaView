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
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVCacheKeeperPipeline();
  ~vtkPVCacheKeeperPipeline() override;

  int ForwardUpstream(int i, int j, vtkInformation* request) override;
  int ForwardUpstream(vtkInformation* request) override;

private:
  vtkPVCacheKeeperPipeline(const vtkPVCacheKeeperPipeline&) = delete;
  void operator=(const vtkPVCacheKeeperPipeline&) = delete;
};

#endif
