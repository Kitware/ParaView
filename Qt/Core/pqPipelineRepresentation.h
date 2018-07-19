/*=========================================================================

   Program: ParaView
   Module:    pqPipelineRepresentation.h

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

=========================================================================*/

/**
* \file pqPipelineRepresentation.h
* \date 4/24/2006
*/

#ifndef _pqPipelineRepresentation_h
#define _pqPipelineRepresentation_h

#include "pqDataRepresentation.h"
#include <QPair>

class pqPipelineSource;
class pqRenderViewModule;
class pqServer;
class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMRepresentationProxy;

/**
* This is PQ representation for a single display. A pqRepresentation represents
* a single vtkSMRepresentationProxy. The display can be added to
* only one render module or more (ofcouse on the same server, this class
* doesn't worry about that.
*/
class PQCORE_EXPORT pqPipelineRepresentation : public pqDataRepresentation
{
  Q_OBJECT
  typedef pqDataRepresentation Superclass;

public:
  // Constructor.
  // \c group :- smgroup in which the proxy has been registered.
  // \c name  :- smname as which the proxy has been registered.
  // \c repr  :- the representation proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqPipelineRepresentation(const QString& group, const QString& name, vtkSMProxy* repr,
    pqServer* server, QObject* parent = NULL);
  ~pqPipelineRepresentation() override;

  // Get the internal display proxy.
  vtkSMRepresentationProxy* getRepresentationProxy() const;

protected:
  // Overridden to set up some additional Qt connections
  void setView(pqView* view) override;

public slots:
  // If lookuptable is set up and is used for coloring,
  // then calling this method resets the table ranges to match the current
  // range of the selected array.
  void resetLookupTableScalarRange();

  // If lookuptable is set up and is used for coloring,
  // then calling this method resets the table ranges to match the
  // range of the selected array over time. This can potentially be a slow
  // processes hence use with caution!!!
  void resetLookupTableScalarRangeOverTime();
};

#endif
