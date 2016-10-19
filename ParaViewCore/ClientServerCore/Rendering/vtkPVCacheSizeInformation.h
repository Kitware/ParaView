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
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*);

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation*);

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //@}

  vtkGetMacro(CacheSize, unsigned long);
  vtkSetMacro(CacheSize, unsigned long);

protected:
  vtkPVCacheSizeInformation();
  ~vtkPVCacheSizeInformation();

  unsigned long CacheSize;

private:
  vtkPVCacheSizeInformation(const vtkPVCacheSizeInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVCacheSizeInformation&) VTK_DELETE_FUNCTION;
};

#endif
