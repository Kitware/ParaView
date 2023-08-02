// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqColorChooserButtonWithPalettes(QWidget* parent = nullptr);
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

private: // NOLINT(readability-redundant-access-specifiers)
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

  Q_DISABLE_COPY(pqColorPaletteLinkHelper)

  vtkWeakPointer<vtkSMProxy> SMProxy;
  QString SMPropertyName;
  friend class pqColorChooserButtonWithPalettes;
};

#endif
