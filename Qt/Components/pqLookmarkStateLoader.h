/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkStateLoader.h

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

#ifndef __pqLookmarkStateLoader_h
#define __pqLookmarkStateLoader_h


#include "pqComponentsExport.h"
//#include "vtkSMPQStateLoader.h"
#include "pqStateLoader.h"
#include <QList>

class pqLookmarkStateLoaderInternal;
class pqPipelineSource;
class pqTimeKeeper;
class QStandardItem;
class pqGenericViewModule;
class pqView;

//
// State loader for the lookmark state.
//

class PQCOMPONENTS_EXPORT pqLookmarkStateLoader : public pqStateLoader
{
public:
  static pqLookmarkStateLoader* New();
  vtkTypeRevisionMacro(pqLookmarkStateLoader, pqStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the state from the given root element. This root
  // element must have Proxy and ProxyCollection sub-elements
  // Returns 1 on success, 0 on failure.
  // If keep_proxies is set, then the internal map
  // of proxy ids to proxies is not cleared on loading of the state.
  //
  // Overloaded to check whether there are enough existing sources to support 
  // this lookmark.
  virtual int LoadState(vtkPVXMLElement* rootElement, int keep_proxies=0);

  // Description:
  // Overloaded to check whether the proxyElement is a source and if so, 
  // look for a display proxy element in the state that has it for an input.
  // Store the id of this display for later so that we know not to load its state.
  virtual vtkSMProxy* NewProxyFromElement(vtkPVXMLElement* proxyElement, int id);

  // Can be given a list of sources to use before any others. 
  // Right now these are the sources that are selected in the pipeline browser.
  void SetPreferredSources(QList<pqPipelineSource*> *sources);

  // The XML representation of the lookmark's pipeline hierarchy.
  // This is used to generate a pipeline mode of the lookmark state which is
  // used by the pqLookmarkSourceDialog
  void SetPipelineHierarchy(vtkPVXMLElement*);

  // Provide access to the timekeeper in case this lookmark restores time
  void SetTimeKeeper(pqTimeKeeper *timekeeper);

  // Set whether or not the lookmark's time/camera should be restored
  void SetRestoreCameraFlag(bool state);
  void SetRestoreTimeFlag(bool state);

  // The view that this lookmark is being displayed in
  void SetView(pqView*);

protected:
  pqLookmarkStateLoader();
  ~pqLookmarkStateLoader();

  // Description:
  // Make sure the "sources" proxy collection gets loaded before any other.
  // When the source collection element does get passed, reorder child elements
  // so that sources get loaded in the same order as their entries in the lookmark 
  // pipeline. This is required for the pqLookmarkSourceDialog to work correctly
  virtual int HandleProxyCollection(vtkPVXMLElement* collectionElement);

  // Description:
  // When a source proxy is about to be created, provide it with an existing one instead.
  virtual vtkSMProxy* NewProxyInternal(const char* xmlgroup, const char* xmlname);

  // Make sure we do not re-register proxies that are being reused or ignored
  virtual void RegisterProxy(int id, vtkSMProxy* proxy);

  // Right now compound proxy definitions get saved with a lookmark when
  // it's created so we do this so loading the lookmark won't load these proxies
  virtual void HandleCompoundProxyDefinitions(vtkPVXMLElement* element);

  // Description:
  // This method is called to load a proxy state. 
  // It handles different proxy types different ways. 
  virtual int LoadProxyState(vtkPVXMLElement* proxyElement, vtkSMProxy* proxy);

  // Helper function for constructing a QAbstractItemModel from the lookmark's 
  // pipeline hierarchy
  void AddChildItems(vtkPVXMLElement *elem, QStandardItem *item);

private:
  pqLookmarkStateLoader(const pqLookmarkStateLoader&); // Not implemented.
  void operator=(const pqLookmarkStateLoader&); // Not implemented.

  pqLookmarkStateLoaderInternal* Internal;
};


#endif

