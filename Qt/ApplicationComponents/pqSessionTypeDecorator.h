// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSessionTypeDecorator_h
#define pqSessionTypeDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"
#include <QObject>

/**
 * @class pqSessionTypeDecorator
 * @brief decorator to show/hide or enable/disable property widget based on the
 *        session.
 *
 * pqSessionTypeDecorator is a pqPropertyWidgetDecorator subclass that can be
 * used to show/hide or enable/disable a pqPropertyWidget based on the current
 * session.
 *
 * The XML config for this decorate takes two attributes 'requires' and 'mode'.
 * 'mode' can have values 'visibility' or 'enabled_state' which dictates whether
 * the decorator shows/hides or enables/disables the widget respectively.
 *
 * 'requires' can have values 'remote', 'parallel', 'parallel_data_server', or
 * 'parallel_render_server' indicating if the session must be remote, or parallel
 * with either data server or render server having more than 1 rank, or parallel
 * data-server, or parallel render-server respectively.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSessionTypeDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqSessionTypeDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqSessionTypeDecorator() override;

  ///@{
  /**
   * Methods overridden from pqPropertyWidget.
   */
  bool canShowWidget(bool show_advanced) const override;
  bool enableWidget() const override;
  ///@}

private:
  Q_DISABLE_COPY(pqSessionTypeDecorator);

  bool IsVisible;
  bool IsEnabled;
};

#endif
