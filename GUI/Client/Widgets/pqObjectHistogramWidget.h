/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

#ifndef _pqObjectHistogramWidget_h
#define _pqObjectHistogramWidget_h

#include "pqVariableType.h"
#include "QtWidgetsExport.h"
#include <QWidget>

class pqServer;
class vtkCommand;
class vtkObject;
class vtkSMProxy;

/// Displays a histogram based on data from a single proxy
class QTWIDGETS_EXPORT pqObjectHistogramWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqObjectHistogramWidget(QWidget* parent);
  ~pqObjectHistogramWidget();

public slots:
  /// Call this to set the current server
  void setServer(pqServer*);
  /// Call this to set the proxy that will become the data source
  void setProxy(vtkSMProxy*);
  /// Call this to set the current variable type and variable name
  void setVariable(pqVariableType type, const QString& name);
  /// Call this to set the current bin count (defaults to 10)
  void setBinCount(unsigned long Count);

private slots:
  void onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*);
  void onBinCountChanged(int);

private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
