/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCacheSizeInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCacheSizeInformation
 * @brief   information obeject to
 * collect cache size information from a vtkCacheSizeKeeper.
 *
 * Gather information about cache size from vtkCacheSizeKeeper.
*/

#ifndef vtkPVCacheSizeInformation_h
#define vtkPVCacheSizeInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVCacheSizeInformation : public vtkPVInformation
{
public:
  static vtkPVCacheSizeInformation* New();
  vtkTypeMacro(vtkPVCacheSizeInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  vtkGetMacro(CacheSize, unsigned long);
  vtkSetMacro(CacheSize, unsigned long);

protected:
  vtkPVCacheSizeInformation();
  ~vtkPVCacheSizeInformation() override;

  unsigned long CacheSize;

private:
  vtkPVCacheSizeInformation(const vtkPVCacheSizeInformation&) = delete;
  void operator=(const vtkPVCacheSizeInformation&) = delete;
};

#endif
