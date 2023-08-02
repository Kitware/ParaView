// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCTHArraySelectionDecorator_h
#define pqCTHArraySelectionDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"

/**
 * pqCTHArraySelectionDecorator is used by ExtractCTH filter (and similar
 * filters) to ensure only 1 of the array-selection properties is set at any
 * given time. When the user changes the state of a selection property,
 * selections on other properties are cleared.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCTHArraySelectionDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqCTHArraySelectionDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqCTHArraySelectionDecorator() override;

private Q_SLOTS:
  void updateSelection();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqCTHArraySelectionDecorator)
  QStringList PropertyNames;
};

#endif
