/*=========================================================================

   Program: ParaView
   Module:    pqChartRepresentation.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef __pqChartRepresentation_h 
#define __pqChartRepresentation_h

#include "pqDataRepresentation.h"

/// pqChartRepresentation is pqDataRepresentation subclass with logic to setup
/// good defaults for chart representations. The default setting logic will
/// eventually move to a controller layer. For now, it sits in the pq-layer to
/// be consistent with the existing framework.
class PQCORE_EXPORT pqChartRepresentation : public pqDataRepresentation
{
  Q_OBJECT
  typedef pqDataRepresentation Superclass;
public:
  pqChartRepresentation(const QString& group, const QString& name,
    vtkSMProxy* reprProxy, pqServer* server,
    QObject* parent=0);
  virtual ~pqChartRepresentation();

  /// Sets default values for the underlying proxy.
  /// This is during the initialization stage of the pqProxy
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called.
  /// The default implementation iterates over all properties
  /// of the proxy and sets them to default values.
  virtual void setDefaultPropertyValues();

  /// Get/set the application setting that is a list of regular expressions to
  /// match chart series that should not initially be visible.
  static void setHiddenSeriesSetting(QStringList list);
  static QStringList getHiddenSeriesSetting();
  static QStringList defaultHiddenSeriesSetting();

  /// Convenience method that determines if a series of the given name should
  /// be hidden by default.
  static bool queryHideSeries(QString array);

private:
  pqChartRepresentation(const pqChartRepresentation&); // Not implemented.
  void operator=(const pqChartRepresentation&); // Not implemented.
};

#endif


