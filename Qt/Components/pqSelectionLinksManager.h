/*=========================================================================

   Program: ParaView
   Module:    pqSelectionLinksManager.h

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
#ifndef __pqSelectionLinksManager_h 
#define __pqSelectionLinksManager_h

#include <QObject>
#include "pqComponentsExport.h"

class pqPipelineSource;
class pqOutputPort;
class pqDataRepresentation;

/// pqSelectionLinksManager is used to create and manage selection links 
/// between different representations of a source (rather an output port 
/// of a source).
class PQCOMPONENTS_EXPORT pqSelectionLinksManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqSelectionLinksManager();
  ~pqSelectionLinksManager();

protected slots:
  /// Called when a source is created.
  /// We initialize observers to know when representations are added to the
  /// source.
  void sourceAdded(pqPipelineSource*);

  /// Called when a source is removed.
  void sourceRemoved(pqPipelineSource*);

  /// Called when a representation is added.
  void representationAdded(pqOutputPort*, pqDataRepresentation*);

  /// Called when a representation is removed.
  void representationRemoved(pqOutputPort*, pqDataRepresentation*);
private:
  pqSelectionLinksManager(const pqSelectionLinksManager&); // Not implemented.
  void operator=(const pqSelectionLinksManager&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif


