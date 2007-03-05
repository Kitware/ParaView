/*=========================================================================

   Program: ParaView
   Module:    pqConsumerDisplay.h

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

=========================================================================*/
#ifndef __pqConsumerDisplay_h
#define __pqConsumerDisplay_h

#include "pqDisplay.h"

class pqConsumerDisplayInternal;
class pqPipelineSource;
class pqScalarsToColors;

// pqConsumerDisplay is the superclass for a display for a pqPiplineSource 
// i.e. the input for this display proxy is a pqPiplineSource.
// This class manages the linking between the pqPiplineSource 
// and pqConsumerDisplay.
class PQCORE_EXPORT pqConsumerDisplay : public pqDisplay
{
  Q_OBJECT
public:
  pqConsumerDisplay(const QString& group, const QString& name,
    vtkSMProxy* display, pqServer* server,
    QObject* parent=0);
  virtual ~pqConsumerDisplay();

  // Get the source/filter of which this is a display.
  pqPipelineSource* getInput() const;


  /// Returns the lookuptable proxy, if any.
  /// Most consumer displays take a lookup table. This method 
  /// provides access to the Lookup table, if one exists.
  virtual vtkSMProxy* getLookupTableProxy();

  /// Returns the pqScalarsToColors object for the lookup table
  /// proxy if any.
  /// Most consumer displays take a lookup table. This method 
  /// provides access to the Lookup table, if one exists.
  virtual pqScalarsToColors* getLookupTable();


  // Called after to creation to set default values.
  virtual void setDefaults();
protected slots:
  // called when input property on display changes. We must detect if
  // (and when) the display is connected to a new proxy.
  virtual void onInputChanged();

private:
  pqConsumerDisplay(const pqConsumerDisplay&); // Not implemented.
  void operator=(const pqConsumerDisplay&); // Not implemented.

  pqConsumerDisplayInternal* Internal;
};

#endif

