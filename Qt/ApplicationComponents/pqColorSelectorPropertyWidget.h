/*=========================================================================

   Program: ParaView
   Module: pqColorSelectorPropertyWidget.h

   Copyright (c) 2005-2012 Kitware Inc.
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

#ifndef _pqColorSelectorPropertyWidget_h
#define _pqColorSelectorPropertyWidget_h

#include "pqApplicationComponentsModule.h"

#include "pqPropertyWidget.h"

/**
* A property widget with a tool button for selecting a single color.
*
* To use this widget for a property add the 'panel_widget="color_selector"' attribute
* to the property's XML. To use this widget for a property whose color might possibly
* come from the global color palette, add the 'panel_widget="color_selector_with_palette"'
* attribute to the property's XML.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorSelectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  pqColorSelectorPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, bool withPalette, QWidget* parent = 0);
  ~pqColorSelectorPropertyWidget();
};

#endif // _pqColorSelectorPropertyWidget_h
