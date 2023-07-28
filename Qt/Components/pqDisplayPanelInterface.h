// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqDisplayPanelInterface_h
#define pqDisplayPanelInterface_h

#include "pqComponentsModule.h"
#include <QtPlugin>
class pqDisplayPanel;
class pqRepresentation;
class QWidget;

/**
 * interface class for plugins that create pqDisplayPanels
 */
class PQCOMPONENTS_EXPORT pqDisplayPanelInterface
{
public:
  /**
   * destructor
   */
  virtual ~pqDisplayPanelInterface();

  /**
   * Returns true if this panel can be created for the given the proxy.
   */
  virtual bool canCreatePanel(pqRepresentation* display) const = 0;
  /**
   * Creates a panel for the given proxy
   */
  virtual pqDisplayPanel* createPanel(pqRepresentation* display, QWidget* parent) = 0;
};

Q_DECLARE_INTERFACE(pqDisplayPanelInterface, "com.kitware/paraview/displaypanel")

#endif
