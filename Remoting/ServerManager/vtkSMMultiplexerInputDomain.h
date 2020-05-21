/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiplexerInputDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMMultiplexerInputDomain
 * @brief input domain used for vtkSMMultiplexerSourceProxy
 *
 * vtkSMMultiplexerInputDomain is intended to be used for input properties on
 * vtkSMMultiplexerSourceProxy. It ensures that `IsInDomain` checks are
 * forwarded for all available multiplexed proxies.
 */

#ifndef vtkSMMultiplexerInputDomain_h
#define vtkSMMultiplexerInputDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMMultiplexerInputDomain : public vtkSMDomain
{
public:
  static vtkSMMultiplexerInputDomain* New();
  vtkTypeMacro(vtkSMMultiplexerInputDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int IsInDomain(vtkSMProperty* property) override;

protected:
  vtkSMMultiplexerInputDomain();
  ~vtkSMMultiplexerInputDomain();

private:
  vtkSMMultiplexerInputDomain(const vtkSMMultiplexerInputDomain&) = delete;
  void operator=(const vtkSMMultiplexerInputDomain&) = delete;
};

#endif
