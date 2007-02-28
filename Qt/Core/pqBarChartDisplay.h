/*=========================================================================

   Program: ParaView
   Module:    pqBarChartDisplay.h

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
#ifndef __pqBarChartDisplay_h
#define __pqBarChartDisplay_h

#include "pqConsumerDisplay.h"

class pqScalarsToColors;
class vtkDataArray;
class vtkRectilinearGrid;
class vtkTimeStamp;

/// pqBarChartDisplay is a pqDisplay for "BarChartDisplay" proxy.
/// It adds logic to initialize the default state of the display proxy
/// as well as managing lookuptable.
class PQCORE_EXPORT pqBarChartDisplay : public pqConsumerDisplay
{
  Q_OBJECT
  typedef pqConsumerDisplay Superclass;
public:
  pqBarChartDisplay(const QString& group, const QString& name,
    vtkSMProxy* display, pqServer* server,
    QObject* parent=0);
  virtual ~pqBarChartDisplay();

  /// Called after to creation to set default values.
  virtual void setDefaults();

  /// Sets up the looktable for the display. It requests the lookuptable
  /// manager for a lookuptable for array with given name and 1 component.
  pqScalarsToColors* setLookupTable(const char* arrayname);

  /// Returns the client-side data array for the X axis
  /// based on the properties set on the display proxy.
  /// Note that this method does not update the pipeline.
  vtkDataArray* getXArray();

  /// Returns the client-side data array for the Y axis
  /// based on the properties set on the display proxy.
  /// Note that this method does not update the pipeline.
  vtkDataArray* getYArray();

  /// Returns the client-side rectilinear grid. 
  /// Note that this method does not update the pipeline.
  vtkRectilinearGrid* getClientSideData() const;

  /// Returns the time when the underlying proxy changed
  /// or the client side data (if any) changed.
  vtkTimeStamp getMTime() const;

public slots:
  /// Updates the lookup table based on the current proxy values.
  void updateLookupTable();

protected slots:
  /// updates MTime.
  void markModified();

private:
  pqBarChartDisplay(const pqBarChartDisplay&); // Not implemented.
  void operator=(const pqBarChartDisplay&); // Not implemented.

  class pqInternals;
  pqInternals* Internal;
};


#endif

