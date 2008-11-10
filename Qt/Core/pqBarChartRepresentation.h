/*=========================================================================

   Program: ParaView
   Module:    pqBarChartRepresentation.h

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
#ifndef __pqBarChartRepresentation_h
#define __pqBarChartRepresentation_h

#include "pqDataRepresentation.h"

class pqScalarsToColors;
class vtkDataArray;
class vtkTable;
class vtkTimeStamp;

/// pqBarChartRepresentation is a pqRepresentation for "BarChartDisplay" proxy.
/// It adds logic to initialize the default state of the display proxy
/// as well as managing lookuptable.
class PQCORE_EXPORT pqBarChartRepresentation : public pqDataRepresentation
{
  Q_OBJECT
  typedef pqDataRepresentation Superclass;
public:
  pqBarChartRepresentation(const QString& group, const QString& name,
    vtkSMProxy* display, pqServer* server,
    QObject* parent=0);
  virtual ~pqBarChartRepresentation();

  /// Sets default values for the underlying proxy. 
  /// This is during the initialization stage of the pqProxy 
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called. 
  /// The default implementation iterates over all properties
  /// of the proxy and sets them to default values. 
  virtual void setDefaultPropertyValues();

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

  /// Returns the component from XArray to plot.
  /// -1 indicates magnitude.
  int getXArrayComponent();

  /// Returns the component from YArray to plot.
  /// -1 indicates magnitude.
  int getYArrayComponent();

  /// Returns the client-side vtkTable.
  /// Note that this method does not update the pipeline.
  vtkTable* getClientSideData() const;

  /// Returns the time when the underlying proxy changed
  /// or the client side data (if any) changed.
  vtkTimeStamp getMTime() const;

public slots:
  /// Updates the lookup table based on the current proxy values.
  void updateLookupTable();

  // Resets the lookup table ranges to match the current range of
  // the selected array.
  void resetLookupTableScalarRange();

protected slots:
  /// updates MTime.
  void markModified();

private:
  pqBarChartRepresentation(const pqBarChartRepresentation&); // Not implemented.
  void operator=(const pqBarChartRepresentation&); // Not implemented.

  class pqInternals;
  pqInternals* Internal;
};


#endif

