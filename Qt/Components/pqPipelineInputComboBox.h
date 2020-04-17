/*=========================================================================

   Program: ParaView
   Module:    pqPipelineInputComboBox.h

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

========================================================================*/
#ifndef pqPipelineInputComboBox_h
#define pqPipelineInputComboBox_h

#include <QComboBox>

#include "pqComponentsModule.h"
#include "vtkSmartPointer.h"

class pqDataRepresentation;
class pqRenderView;

class vtkSMProxy;
class vtkSMProperty;

/**
* This is a ComboBox that is used on the display tab to select available
* pipeline inputs.
*/
class PQCOMPONENTS_EXPORT pqPipelineInputComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqPipelineInputComboBox(vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = NULL);
  ~pqPipelineInputComboBox() override;

  vtkSMProxy* currentProxy() const;

public Q_SLOTS:
  void setCurrentProxy(vtkSMProxy* proxy);

  /**
  * Forces a reload of the widget. Generally one does not need to call this
  * method explicity.
  */
  void reload();

protected Q_SLOTS:
  /**
  * Called when user activates an item.
  */
  void onActivated(int);

  void proxyRegistered(const QString& group, const QString& name, vtkSMProxy* proxy);
  void proxyUnRegistered(const QString& group, const QString& name, vtkSMProxy* proxy);

private:
  Q_DISABLE_COPY(pqPipelineInputComboBox)

  vtkSmartPointer<vtkSMProxy> Proxy;
  vtkSmartPointer<vtkSMProperty> Property;

  bool UpdatePending;
  bool InOnActivate;
};

#endif
