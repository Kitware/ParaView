/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkStateLoader.h

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

#ifndef __pqLookmarkStateLoader_h
#define __pqLookmarkStateLoader_h


#include "pqCoreExport.h"
#include "vtkSMPQStateLoader.h"
#include <QList>

//class pqMainWindowCore;
class pqLookmarkStateLoaderInternal;
class pqPipelineSource;

// State loader for the lookmark state.
// Currently only supports state with single (non-filter) sources. 
// When UseDataFlag is turned off, the state of the stored (non-filter) source proxy is used to set up the active (non-filter) source in the pipeline browser.
// When UseCameraFlag is turned off, the camera properties of the render module proxy are filtered out.
// TODO: Support multiple sources. As they are encountered, prompt user to select which one to use for the given proxy.
// Will also need to prompt the user for the case when the lookmark's state has only one (non-filter) source but the active pipeline has multiple. 
class PQCORE_EXPORT pqLookmarkStateLoader : public vtkSMPQStateLoader
{
public:
  static pqLookmarkStateLoader* New();
  vtkTypeRevisionMacro(pqLookmarkStateLoader, vtkSMPQStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // A root xml element is created before passing  the xml data structure up for the superclass to handle
  virtual int LoadState(vtkPVXMLElement* rootElement, int keep_proxies=0);

  // Set the main window core. The core is GUI side manager.
  // This is used to grab the active (non-filter) source in the pipeline
  //void SetMainWindowCore(pqMainWindowCore* core);

  // When UseDataFlag is turned off, the state of the stored (non-filter) source proxy is used to set up the active (non-filter) source in the pipeline browser.
  void SetUseDataFlag(bool state);

  // When UseCameraFlag is turned off, the camera properties of the render module proxy are filtered out.
  void SetUseCameraFlag(bool state);

  //void SetMultipleSourcesFlag(bool state);

protected:
  pqLookmarkStateLoader();
  ~pqLookmarkStateLoader();

  // Description:
  // Overridden so that animation scene proxy is not recreated types.
  virtual vtkSMProxy* NewProxyInternal(const char* xmlgroup, const char* xmlname);

  // Overridden to avoid registering the reused animation scene twice.
  virtual void RegisterProxyInternal(const char* group, 
    const char* name, vtkSMProxy* proxy);

  // Description:
  // This method is called to load a proxy state. Overloaded to make sure that 
  // when states are loaded for existsing render modules, the displays already present
  // in those render modules are not affected.
  virtual int LoadProxyState(vtkPVXMLElement* proxyElement, vtkSMProxy* proxy);

  /// populate list of the non-server, non-filter objects that are in the same pipeline as "src"
  void GetRootSources(QList<pqPipelineSource*> *sources, pqPipelineSource *src);

private:
  pqLookmarkStateLoader(const pqLookmarkStateLoader&); // Not implemented.
  void operator=(const pqLookmarkStateLoader&); // Not implemented.

  pqLookmarkStateLoaderInternal* Internal;
};


#endif

