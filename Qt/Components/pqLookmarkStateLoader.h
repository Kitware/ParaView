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


#include "pqComponentsExport.h"
#include "vtkSMPQStateLoader.h"
#include <QList>

class pqLookmarkStateLoaderInternal;
class pqPipelineSource;
class QStandardItem;

// State loader for the lookmark state.
// Supports 
// When UseCameraFlag is turned off, the camera properties of the render module proxy are filtered out.

class PQCOMPONENTS_EXPORT pqLookmarkStateLoader : public vtkSMPQStateLoader
{
public:
  static pqLookmarkStateLoader* New();
  vtkTypeRevisionMacro(pqLookmarkStateLoader, vtkSMPQStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int LoadState(vtkPVXMLElement* rootElement, int keep_proxies=0);

  // Can be given a list of sources to use before any others
  void SetPreferredSources(QList<pqPipelineSource*> *sources);

  // The XML representation of the lookmark's pipeline hierarchy.
  // see pqLookmarkModel
  void SetPipelineHierarchy(vtkPVXMLElement*);

  void SetRestoreCameraFlag(bool state);

protected:
  pqLookmarkStateLoader();
  ~pqLookmarkStateLoader();

  // Description:
  // When a source proxy is about to be created, provide it with an existing one instead.
  virtual vtkSMProxy* NewProxyInternal(const char* xmlgroup, const char* xmlname);

  // Make sure we do not re-register proxies that are being reused
  virtual void RegisterProxyInternal(const char* group, 
    const char* name, vtkSMProxy* proxy);

  virtual void HandleCompoundProxyDefinitions(vtkPVXMLElement* element);

  // Description:
  // This method is called to load a proxy state. Overloaded to make sure their are enough existing source for this lookmark
  virtual int LoadProxyState(vtkPVXMLElement* proxyElement, vtkSMProxy* proxy);

  // Helper function for constructing a QAbstractItemModel from the lookmark's pipeline hierarchy
  void AddChildItems(vtkPVXMLElement *elem, QStandardItem *item);

private:
  pqLookmarkStateLoader(const pqLookmarkStateLoader&); // Not implemented.
  void operator=(const pqLookmarkStateLoader&); // Not implemented.

  pqLookmarkStateLoaderInternal* Internal;
};


#endif

