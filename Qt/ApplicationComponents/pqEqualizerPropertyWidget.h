/*=========================================================================

  Program:   ParaView
  Module:    pqEqualizerPropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef pqEqualizerPropertyWidget_h
#define pqEqualizerPropertyWidget_h

#include "pqInteractiveProperty2DWidget.h"

#include <QScopedPointer>

/**
 * @class pqEqualizerPropertyWidget
 * @brief The pqEqualizerPropertyWidget class
 *
 * To use this widget for a property group (vtkSMPropertyGroup),
 * use "EqualizerPropertyWidget" as the "panel_widget" in the
 * XML configuration for the proxy.
 * It controls the "EqualizerPoints" function of the given property group.
 * It is also possible to give an optional function "SamplingFrequency"
 * that will be used for initializing the placement of the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEqualizerPropertyWidget
  : public pqInteractiveProperty2DWidget
{
  Q_OBJECT
  typedef pqInteractiveProperty2DWidget Superclass;

public:
  explicit pqEqualizerPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqEqualizerPropertyWidget();

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;
  void updatePosition();

private Q_SLOTS:
  void saveEqualizer();
  void loadEqualizer();
  void resetEqualizer();

private:
  Q_DISABLE_COPY(pqEqualizerPropertyWidget)

  pqPropertyLinks WidgetLinks;

  struct pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
