/*=========================================================================

   Program: ParaView
   Module:  pqColorPaletteSelectorWidget.h

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
#ifndef pqColorPaletteSelectorWidget_h
#define pqColorPaletteSelectorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QPointer>

class QComboBox;

/**
 * @class pqColorPaletteSelectorWidget
 * @brief widget to choose a color palette to load/select.
 *
 * pqColorPaletteSelectorWidget is a pqPropertyWidget intended to be used in two
 * roles:
 * 1. To load a specific color palette.
 * 2. To select a specific color palette.
 *
 * Mode (1) is used when the widget is used for a `vtkSMProperty` e.g. **'Load
 * Palette'** property on the **ColorPalette** proxy. In that case, the
 * user's action is expected to update the proxy with the chosen palette.
 *
 * Mode (2) is used when the widget is used for a `vtkSMStringVectorProperty`
 * e.g. **OverrideColorPalette** property on **ImageOptions** proxy. In that
 * case, the selected palette name is simply set on the
 * vtkSMStringVectorProperty.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorPaletteSelectorWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QString paletteName READ paletteName WRITE setPaletteName)
  typedef pqPropertyWidget Superclass;

public:
  pqColorPaletteSelectorWidget(vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parent = 0);
  ~pqColorPaletteSelectorWidget() override;

  QString paletteName() const;
  void setPaletteName(const QString& name);

Q_SIGNALS:
  void paletteNameChanged();

private Q_SLOTS:
  void loadPalette(int);

private:
  Q_DISABLE_COPY(pqColorPaletteSelectorWidget)
  QPointer<QComboBox> ComboBox;
};

#endif
