/*=========================================================================

   Program: ParaView
   Module:  pqColorChooserButtonWithPalettes.h

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
#ifndef pqColorChooserButtonWithPalettes_h
#define pqColorChooserButtonWithPalettes_h

#include "pqColorChooserButton.h"
#include "pqComponentsModule.h"
#include "vtkWeakPointer.h"
#include <QPointer>

class QActionGroup;
class vtkSMSettingsProxy;
class vtkSMProxy;

//============================================================================
/**
* pqColorChooserButtonWithPalettes extends pqColorChooserButton to add support
* for a menu that allows the user to connect the color to a color in the
* ParaView application's color palettes.
*
* When the user selects a color from the color palette, this class will get
* the color value from the palette and simply apply it, as if the user
* explicitly chose the color.
*
* However, in ParaView, when the user selects a color from the palette,
* there's an expectation that the color stays "linked" with the color palette.
* Thus, if the color palette is changed, the linked color is also updated.
*
* To achieve that, pqColorChooserButtonWithPalettes is often used with a
* pqColorPaletteLinkHelper. Simply instantiate a pqColorPaletteLinkHelper
* instance with appropriate constructor arguments and then linking of
* properties with the color palette is automatically managed.
*/
class PQCOMPONENTS_EXPORT pqColorChooserButtonWithPalettes : public pqColorChooserButton
{
  Q_OBJECT
  typedef pqColorChooserButton Superclass;

public:
  pqColorChooserButtonWithPalettes(QWidget* parent = 0);
  ~pqColorChooserButtonWithPalettes() override;

private Q_SLOTS:
  /**
  * Called to rebuild the menu. This is called everytime the popup menu is
  * going to be shown.
  */
  void updateMenu();

  /**
  * callback when a menu action is triggered.
  */
  void actionTriggered(QAction*);

private:
  Q_DISABLE_COPY(pqColorChooserButtonWithPalettes)

  /**
  * Returns the color palette proxy for the active session.
  */
  vtkSMSettingsProxy* colorPalette() const;
  QPointer<QActionGroup> ActionGroup;
  friend class pqColorPaletteLinkHelper;
};

//============================================================================
/**
* pqColorPaletteLinkHelper is designed to be used with
* pqColorChooserButtonWithPalettes to manage setting up of property links with
* ParaView application's color palette.
*/
class PQCOMPONENTS_EXPORT pqColorPaletteLinkHelper : public QObject
{
  Q_OBJECT;
  typedef QObject Superclass;

public:
  pqColorPaletteLinkHelper(
    pqColorChooserButtonWithPalettes* button, vtkSMProxy* smproxy, const char* smproperty);
  ~pqColorPaletteLinkHelper() override;

private:
  /**
  * called when a palette color is selected by the user.
  */
  void setSelectedPaletteColor(const QString& colorName);

  /**
  * Returns the name of the current selected palette color for the
  * smproperty, if any.
  */
  QString selectedPaletteColor() const;

private:
  Q_DISABLE_COPY(pqColorPaletteLinkHelper)

  vtkWeakPointer<vtkSMProxy> SMProxy;
  QString SMPropertyName;
  friend class pqColorChooserButtonWithPalettes;
};

#endif
