/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineDisplay.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqPipelineDisplay.h
/// \date 4/24/2006

#ifndef _pqPipelineDisplay_h
#define _pqPipelineDisplay_h


#include "pqPipelineObject.h"

class pqPipelineDisplayInternal;
class pqPipelineSource;
class pqServer;
class vtkSMDataObjectDisplayProxy;

/// This is PQ representation for a single display. A pqDisplay represents
/// a single vtkSMDataObjectDisplayProxy. The display can be added to
/// only one render module or more (ofcouse on the same server, this class
/// doesn't worry about that.
class PQWIDGETS_EXPORT pqPipelineDisplay : public pqPipelineObject
{
public:
  pqPipelineDisplay(vtkSMDataObjectDisplayProxy* display, pqServer* server,
    QObject* parent=NULL);
  virtual ~pqPipelineDisplay();

  // Get the internal display proxy.
  vtkSMDataObjectDisplayProxy* getProxy() const;

  // Get the source/filter of which this is a display.
  pqPipelineSource* getInput() const;

  // Sets the default color mapping for the display.
  // The rules are:
  // If the source created a NEW point scalar array, use it.
  // Else if the source created a NEW cell scalar array, use it.
  // Else if the input color by array exists in this source, use it.
  // Else color by property.
  void setDefaultColorParametes(); 
protected slots:
  // called when input property on display changes. We must detect if
  // (and when) the display is connected to a new proxy.
  virtual void onInputChanged();

private:
  pqPipelineDisplayInternal *Internal; 
};

#endif
