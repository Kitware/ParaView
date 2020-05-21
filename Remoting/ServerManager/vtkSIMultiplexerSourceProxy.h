/*=========================================================================

  Program:   ParaView
  Module:    vtkSIMultiplexerSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSIMultiplexerSourceProxy
 * @brief vtkSIProxy subclass for vtkSMMultiplexerSourceProxy
 *
 * vtkSIMultiplexerSourceProxy is intended for use with
 * vtkSMMultiplexerSourceProxy. It adds API to activate a subproxy to act as the
 * data producer.
 */

#ifndef vtkSIMultiplexerSourceProxy_h
#define vtkSIMultiplexerSourceProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSISourceProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIMultiplexerSourceProxy : public vtkSISourceProxy
{
public:
  static vtkSIMultiplexerSourceProxy* New();
  vtkTypeMacro(vtkSIMultiplexerSourceProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called to select one of the subproxies as the active one.
   */
  void Select(vtkSISourceProxy* subproxy);

protected:
  vtkSIMultiplexerSourceProxy();
  ~vtkSIMultiplexerSourceProxy();

private:
  vtkSIMultiplexerSourceProxy(const vtkSIMultiplexerSourceProxy&) = delete;
  void operator=(const vtkSIMultiplexerSourceProxy&) = delete;
};

#endif
