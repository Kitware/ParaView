/*=========================================================================

   Program: ParaView
   Module: pqTextureSelectorPropertyWidget.h

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

#ifndef _pqTextureSelectorPropertyWidget_h
#define _pqTextureSelectorPropertyWidget_h

#include "pqComponentsModule.h"

#include "pqPropertyWidget.h"
#include "vtkNew.h"

/**
* Property widget for selecting the texture to apply to a surface.
*
* To use this widget for a property add the 'panel_widget="texture_selector"'
* to the property's XML.
*/
class vtkSMProxyGroupDomain;
class pqTextureComboBox;
class pqDataRepresentation;
class PQCOMPONENTS_EXPORT pqTextureSelectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  pqTextureSelectorPropertyWidget(vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = 0);
  ~pqTextureSelectorPropertyWidget() override = default;

protected Q_SLOTS:
  void onTextureChanged(vtkSMProxy* texture);
  void onPropertyChanged();
  void checkAttributes(bool tcoords, bool tangents);

private:
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  pqTextureComboBox* Selector;
  vtkSMProxyGroupDomain* Domain;
  pqDataRepresentation* Representation = nullptr;
  pqView* View = nullptr;
};

#endif // _pqTextureSelectorPropertyWidget_h
