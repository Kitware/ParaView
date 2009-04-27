/*=========================================================================

   Program: ParaView
   Module:    pqComparativePlotView.h

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
#ifndef __pqComparativePlotView_h
#define __pqComparativePlotView_h

#include "pqPlotView.h"

class vtkSMComparativeViewProxy;

/// PlotView used for comparative visualization (or film-strip visualization).
/// THIS CLASS HAS BEEN DEPRECATED. IT'S NO LONGER USED. IT HAS BEEN REPLACED BY
/// pqComparativeChartView AND SUBCLASSES. THIS CLASS IS LEFT AROUND UNTIL ALL
//THE FUNCTIONALITY HAS MOVED TO THE NEW CLASSES.
class PQCORE_EXPORT pqComparativePlotView : public pqPlotView
{
  Q_OBJECT
public:
  typedef pqPlotView Superclass;

  static QString comparativeXYPlotViewType() { return "ComparativeXYPlotView"; }
  static QString comparativeXYPlotViewTypeName() { return "XY Plot View (Comparative)"; }
  static QString comparativeBarChartViewType() { return "ComparativeBarChartView"; }
  static QString comparativeBarChartViewTypeName() { return "Bar Chart View (Comparative)"; }

  // Constructor:
  // \c type  :- Specific PlotView subclass type (ex, XYPlotView)
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- PlotView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqComparativePlotView(const QString& type,
                const QString& group,
                const QString& name, 
                vtkSMViewProxy* renModule, 
                pqServer* server,
                QObject* parent=NULL);
  virtual ~pqComparativePlotView();

  /// Returns the comparative view proxy.
  vtkSMComparativeViewProxy* getComparativeViewProxy() const;

  /// Returns the proxy of the root plot view in the comparative view.
  virtual vtkSMViewProxy* getViewProxy() const;

  /// Sets default values for the underlying proxy. 
  /// This is during the initialization stage of the pqProxy 
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called. 
  virtual void setDefaultPropertyValues();

public slots:

  void representationsChanged();

protected slots:
  /// Called when the layout on the comparative vis changes.
  void onComparativeVisLayoutChanged();

  /// Update the visibility of all plots in comparative view
  void updateVisibility();

  /// This method may be used to adjust the chart title
  /// text by inserting comparative variable values or
  /// time values.
  virtual void adjustTitleText(const pqPlotView *, QString &);

protected:

  /// Use this method to initialize the pqObject state using the
  /// underlying vtkSMProxy. This needs to be done only once,
  /// after the object has been created. 
  virtual void initialize();

private:
  pqComparativePlotView(const pqComparativePlotView&); // Not implemented.
  void operator=(const pqComparativePlotView&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};


#endif

