/*=========================================================================

   Program: ParaView
   Module:    pqStateLoader.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#ifndef __pqStateLoader_h
#define __pqStateLoader_h


#include "pqComponentsExport.h"
#include "vtkSMPQStateLoader.h"

class pqMainWindowCore;
class pqStateLoaderInternal;

// State loader which makes it possible to load
// additional GUI related state (including multiview layout).
class PQCOMPONENTS_EXPORT pqStateLoader : public vtkSMPQStateLoader
{
public:
  static pqStateLoader* New();
  vtkTypeRevisionMacro(pqStateLoader, vtkSMPQStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Loads the GUI as well as the ServerManager state.
  virtual int LoadState(vtkPVXMLElement* rootElement, int keep_proxies=0);

  // Set the main window core. The core is GUI side manager.
  void SetMainWindowCore(pqMainWindowCore* core);
protected:
  pqStateLoader();
  ~pqStateLoader();

  // Description:
  // This method is called to load a proxy state. Overloaded to make sure that 
  // when states are loaded for existsing render modules, the displays already present
  // in those render modules are not affected.
  virtual int LoadProxyState(vtkPVXMLElement* proxyElement, vtkSMProxy* proxy);
private:
  pqStateLoader(const pqStateLoader&); // Not implemented.
  void operator=(const pqStateLoader&); // Not implemented.

  pqStateLoaderInternal* Internal;
};


#endif

