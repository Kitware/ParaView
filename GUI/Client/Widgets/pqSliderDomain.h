/*=========================================================================

   Program:   ParaQ
   Module:    pqSliderDomain.h

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

#ifndef pq_SliderDomain_h
#define pq_SliderDomain_h

#include <QObject>
#include "pqWidgetsExport.h"

class QSlider;
class vtkSMProperty;

/// combo box domain 
/// observers the domain for a combo box and updates accordingly
class PQWIDGETS_EXPORT pqSliderDomain : public QObject
{
  Q_OBJECT
  Q_PROPERTY(double ScaleFactor READ scaleFactor WRITE setScaleFactor)
public:
  /// constructor requires a QSlider, 
  /// and the property with the domain to observe
  /// the list of values in the combo box is automatically 
  /// updated when the domain changes
  pqSliderDomain(QSlider* p, vtkSMProperty* prop, int index=-1);
  ~pqSliderDomain();

  /// set the scale factor
  void setScaleFactor(double scale);
  double scaleFactor() const;

public slots:
  void domainChanged();

protected:
  class pqInternal;
  pqInternal* Internal;
};

#endif

