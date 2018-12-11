/*=========================================================================

  Program:   ParaView
  Module:    vtkCacheSizeKeeper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCacheSizeKeeper
 * @brief   keeps track of amount of memory consumed
 * by caches in vtkPVUpateSupressor objects.
 *
 * vtkCacheSizeKeeper keeps track of the amount of memory cached
 * by several vtkPVUpdateSuppressor objects.
*/

#ifndef vtkCacheSizeKeeper_h
#define vtkCacheSizeKeeper_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkCacheSizeKeeper : public vtkObject
{
public:
  vtkTypeMacro(vtkCacheSizeKeeper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the singleton.
   */
  static vtkCacheSizeKeeper* GetInstance();

  //@{
  /**
   * Report increase in cache size (in kbytes).
   */
  void AddCacheSize(unsigned long kbytes)
  {
    if (this->CacheFull)
    {
      vtkErrorMacro("Cache is full. Cannot add more cached data.");
    }
    else
    {
      this->CacheSize += kbytes;
    }
  }
  //@}

  /**
   * Report decrease in cache size (in bytes).
   */
  void FreeCacheSize(unsigned long kbytes)
  {
    this->CacheSize = (this->CacheSize > kbytes) ? (this->CacheSize - kbytes) : 0;
  }

  //@{
  /**
   * Get the size of cache reported to this keeper.
   */
  vtkGetMacro(CacheSize, unsigned long);
  //@}

  //@{
  /**
   * Get/Set the cache size limit. One can set this separately on each
   * processes. vtkPVView::Update ensures that the cache fullness state is
   * synchronized among all participating processes. (in KBs)
   */
  vtkGetMacro(CacheLimit, unsigned long);
  vtkSetMacro(CacheLimit, unsigned long);
  //@}

  //@{
  /**
   * Get/Set if the cache is full.
   */
  vtkGetMacro(CacheFull, int);
  vtkSetMacro(CacheFull, int);
  //@}

protected:
  static vtkCacheSizeKeeper* New();
  vtkCacheSizeKeeper();
  ~vtkCacheSizeKeeper() override;

  unsigned long CacheSize;
  unsigned long CacheLimit;
  int CacheFull;

private:
  vtkCacheSizeKeeper(const vtkCacheSizeKeeper&) = delete;
  void operator=(const vtkCacheSizeKeeper&) = delete;
};

#endif
