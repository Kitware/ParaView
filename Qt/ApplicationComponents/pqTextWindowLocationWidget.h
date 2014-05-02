/*=========================================================================

   Program: ParaView
   Module:  pqTextWindowLocationWidget.h

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
#ifndef __pqTextWindowLocationWidget_h
#define __pqTextWindowLocationWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

class vtkSMPropertyGroup;

/// pqTextWindowLocationWidget is a pqPropertyWidget that can be used to set
/// the location of the a text representation relative to the viewport.
class PQAPPLICATIONCOMPONENTS_EXPORT pqTextWindowLocationWidget :
  public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QString windowLocation READ windowLocation WRITE setWindowLocation)
//  Q_PROPERTY(QString position1X READ position1X WRITE setPosition1X)
//  Q_PROPERTY(QString position1Y READ position1Y WRITE setPosition1Y)
  
  typedef pqPropertyWidget Superclass;

public:
  pqTextWindowLocationWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent=0);
  virtual ~pqTextWindowLocationWidget();

  QString windowLocation() const;
//  double position1X() const;
//  double position1Y() const;

signals:
  void windowLocationChanged(QString&);
  void position1Changed(double, double);

protected:
  void setWindowLocation(QString&);
//  void setPosition1X(double);
//  void setPosition1Y(double);

protected slots:
  void emitWindowLocationChangedSignal();
  void groupBoxLocationToggled(bool);
  void groupBoxPositionToggled(bool);

private:
  Q_DISABLE_COPY(pqTextWindowLocationWidget)
  class pqInternals;
  pqInternals* Internals;
};

#endif //__pqTextWindowLocationWidget_h
