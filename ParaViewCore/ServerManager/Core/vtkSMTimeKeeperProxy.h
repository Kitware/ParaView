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
// .NAME vtkSMTimeKeeperProxy
// .SECTION Description
// We simply pass the TimestepValues and TimeRange properties to the client-side
// vtkSMTimeKeeper instance so that it can keep those up-to-date.

#ifndef __vtkSMTimeKeeperProxy_h
#define __vtkSMTimeKeeperProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMTimeKeeperProxy : public vtkSMProxy
{
public:
  static vtkSMTimeKeeperProxy* New();
  vtkTypeMacro(vtkSMTimeKeeperProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Track timesteps provided by a source. If \c suppress_input is true, before
  // adding the proxy, if the \r proxy has producers those will be removed from
  // the time sources i.e. we'll ignore timesteps from the input.
  virtual bool AddTimeSource(vtkSMProxy* proxy, bool suppress_input);
  static bool AddTimeSource(vtkSMProxy* timeKeeper, vtkSMProxy* proxy, bool suppress_input)
    {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self? self->AddTimeSource(proxy, suppress_input) : false;
    }

  // Description:
  // Remove a particular time source.
  virtual bool RemoveTimeSource(vtkSMProxy* proxy, bool unsuppress_input);
  static bool RemoveTimeSource(vtkSMProxy* timeKeeper, vtkSMProxy* proxy, bool unsuppress_input)
    {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self? self->RemoveTimeSource(proxy, unsuppress_input) : false;
    }

  // Description:
  // Returns true if the proxy has been added to time sources and not
  // suppressed.
  virtual bool IsTimeSourceTracked(vtkSMProxy* proxy);
  static bool IsTimeSourceTracked(vtkSMProxy* timeKeeper, vtkSMProxy* proxy)
    {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self? self->IsTimeSourceTracked(proxy) : false;
    }

  // Description:
  // Set whether to suppress a time source that has been added to the time
  // keeper. Suppressing a source results in its time being ignored by the time
  // keeper.
  virtual bool SetSuppressTimeSource(vtkSMProxy* proxy, bool suppress);
  static bool SetSuppressTimeSource(vtkSMProxy* timeKeeper, vtkSMProxy* proxy, bool suppress)
    {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self? self->SetSuppressTimeSource(proxy, suppress) : false;
    }


//BTX
protected:
  vtkSMTimeKeeperProxy();
  ~vtkSMTimeKeeperProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMTimeKeeperProxy(const vtkSMTimeKeeperProxy&); // Not implemented
  void operator=(const vtkSMTimeKeeperProxy&); // Not implemented
//ETX
};

#endif
