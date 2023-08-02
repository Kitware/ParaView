// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDisplayPanel_h
#define pqDisplayPanel_h

#include "pqComponentsModule.h"
#include "pqRepresentation.h"
#include <QPointer>
#include <QWidget>

/**
 * Widget which provides an editor for the properties of a
 * representation.
 */
class PQCOMPONENTS_EXPORT pqDisplayPanel : public QWidget
{
  Q_OBJECT
public:
  /**
   * constructor
   */
  pqDisplayPanel(pqRepresentation* Representation, QWidget* p = nullptr);
  /**
   * destructor
   */
  ~pqDisplayPanel() override;

  /**
   * get the proxy for which properties are displayed
   */
  pqRepresentation* getRepresentation();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * TODO: get rid of this function once the server manager can
   * inform us of Representation property changes
   */
  virtual void reloadGUI();

  /**
   * Requests update on all views the Representation is visible in.
   */
  virtual void updateAllViews();

  /**
   * Called when the data information has changed.
   */
  virtual void dataUpdated();

protected:
  QPointer<pqRepresentation> Representation;
};

#endif
