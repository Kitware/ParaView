/*=========================================================================

   Program: ParaView
   Module:    pqComparativeTracksWidget.h

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

========================================================================*/
#ifndef __pqComparativeTracksWidget_h 
#define __pqComparativeTracksWidget_h

#include "pqComponentsExport.h"
#include <QWidget>

class vtkSMProxy;
class vtkSMProperty;

/// Widget for showing the comparative vis parameters.
class PQCOMPONENTS_EXPORT pqComparativeTracksWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqComparativeTracksWidget(QWidget* parent);
  virtual ~pqComparativeTracksWidget();

  /// Set the comparative view proxy.
  void setComparativeView(vtkSMProxy* comparativeViewProxy);

protected slots:
  void updateSceneCallback();
  void updateScene();

protected:
  // update the animation track at the given index using the given "Cues" property
  // from the CVProxy.
  void updateTrack(int index, vtkSMProperty* property);

private:
  pqComparativeTracksWidget(const pqComparativeTracksWidget&); // Not implemented.
  void operator=(const pqComparativeTracksWidget&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif


