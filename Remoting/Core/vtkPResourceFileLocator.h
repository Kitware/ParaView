/*=========================================================================

  Program:   ParaView
  Module:    vtkPResourceFileLocator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPResourceFileLocator
 * @brief parallel aware vtkResourceFileLocator
 *
 * vtkPResourceFileLocator extends vtkResourceFileLocator to support distributed
 * use-cases.
 *
 * vtkResourceFileLocator supports API to locate files on disk. When running in
 * parallel, we don't want to the lookup to happen on all ranks, only the root
 * node and the result to be shared with all ranks. vtkPResourceFileLocator
 * overrides superclass API to handle that.
 */

#ifndef vtkPResourceFileLocator_h
#define vtkPResourceFileLocator_h

#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkResourceFileLocator.h"

class VTKREMOTINGCORE_EXPORT vtkPResourceFileLocator : public vtkResourceFileLocator
{
public:
  static vtkPResourceFileLocator* New();
  vtkTypeMacro(vtkPResourceFileLocator, vtkResourceFileLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::Locate;
  std::string Locate(const std::string& anchor, const std::vector<std::string>& landmark_prefixes,
    const std::string& landmark, const std::string& defaultDir = std::string()) override;

protected:
  vtkPResourceFileLocator();
  ~vtkPResourceFileLocator();

private:
  vtkPResourceFileLocator(const vtkPResourceFileLocator&) = delete;
  void operator=(const vtkPResourceFileLocator&) = delete;
};

#endif
