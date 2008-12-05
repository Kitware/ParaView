/*=========================================================================

   Program: ParaView
   Module:    pqStateLoader.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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
#include "vtkSMStateLoader.h"

class pqMainWindowCore;
class pqStateLoaderInternal;

/// State loader which makes it possible to load
/// additional GUI related state (including multiview layout).
/// This also ensures that the time-keeper and animation scene proxies are not
/// recreated (but existing ones are used).
class PQCOMPONENTS_EXPORT pqStateLoader : public vtkSMStateLoader
{
public:
  static pqStateLoader* New();
  vtkTypeRevisionMacro(pqStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Set the main window core. The core is GUI side manager.
  void SetMainWindowCore(pqMainWindowCore* core);
protected:
  pqStateLoader();
  ~pqStateLoader();

  /// Load the state.
  virtual int LoadStateInternal(vtkPVXMLElement* rootElement);

  /// Description:
  /// Locate the XML for the proxy with the given id. Overridden to filter the
  /// XML for certain proxies.
  virtual vtkPVXMLElement* LocateProxyElement(int id);

  /// Overridden so that animation scene proxy is not recreated types.
  virtual vtkSMProxy* CreateProxy(
    const char* xmlgroup, const char* xmlname, vtkIdType cid);

  /// Overridden to avoid registering the reused animation scene twice.
  virtual void RegisterProxyInternal(const char* group, 
    const char* name, vtkSMProxy* proxy);

  /// Overridden to process pq_helper_proxies groups.
  virtual int BuildProxyCollectionInformation(vtkPVXMLElement*);

  /// Finds helper proxies for any pqProxies and assigns them accordingly.
  void DiscoverHelperProxies();
private:
  pqStateLoader(const pqStateLoader&); // Not implemented.
  void operator=(const pqStateLoader&); // Not implemented.

  pqStateLoaderInternal* Internal;
};


#endif

