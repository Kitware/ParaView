/*=========================================================================

   Program: ParaView
   Module:    pqLineChartRepresentation.h

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
#ifndef __pqLineChartRepresentation_h
#define __pqLineChartRepresentation_h

#include "pqDataRepresentation.h"

class vtkSMProperty;
class vtkDataArray;
class vtkRectilinearGrid;
class QColor;

/// pqLineChartRepresentation is a pqDisplay for "XYPlotDisplay2" proxy.
/// It adds logic to initialize default state as well as access
/// get information about the plot parameters from the proxy.
class PQCORE_EXPORT pqLineChartRepresentation : public pqDataRepresentation
{
  Q_OBJECT
  typedef pqDataRepresentation Superclass;
public:
  pqLineChartRepresentation(const QString& group, const QString& name,
    vtkSMProxy* display, pqServer* server,
    QObject* parent=0);
  virtual ~pqLineChartRepresentation();

  /// Sets default values for the underlying proxy. 
  /// This is during the initialization stage of the pqProxy 
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called. 
  /// The default implementation iterates over all properties
  /// of the proxy and sets them to default values. 
  virtual void setDefaultPropertyValues();

  /// Returns the client-side rectilinear grid. 
  /// Note that this method does not update the pipeline.
  vtkRectilinearGrid* getClientSideData() const;

  /// Returns the array used for x-axis.
  vtkDataArray* getXArray();

  /// Returns the array used for y axis at the given index.
  vtkDataArray* getYArray(int index);

  /// Returns the number of y axis arrays.
  int getNumberOfYArrays() const;

  /// Returns if the y axis array at a particular index is 
  /// currently enabled for plotting.
  bool getYArrayEnabled(int index) const;

  /// Returns the user selected color for the Y array at the given index.
  QColor getYColor(int index) const;

  /// Returns the enable state for array with the given name.
  /// If status for such an array is not present on the property,
  /// returns false.
  bool getYArrayEnabled(const QString& arrayname) const;

  /// Returns the color for array with the given name. 
  /// If no such array is present in the property
  /// returns a random color.
  QColor getYColor(const QString& arrayname) const;
protected:
  /// method to set default values for the status property.
  void setStatusDefaults(vtkSMProperty* prop);

private:
  pqLineChartRepresentation(const pqLineChartRepresentation&); // Not implemented.
  void operator=(const pqLineChartRepresentation&); // Not implemented.

  class pqInternals;
  pqInternals *Internals;
};


#endif

