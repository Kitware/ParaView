/*=========================================================================

   Program: ParaView
   Module:  pqDisplaySizedImplicitPlanePropertyWidget.h

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
#ifndef pqDisplaySizedImplicitPlanePropertyWidget_h
#define pqDisplaySizedImplicitPlanePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

/**
 * pqDisplaySizedImplicitPlanePropertyWidget is a custom property widget that uses
 * "DisplaySizedImplicitPlaneWidgetRepresentation" to help users interactively set the origin
 * and normal for a plane. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractivePlane" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c Origin: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * origin of the interactive plane.
 * \li \c Normal: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * normal for the interactive plane.
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqDisplaySizedImplicitPlanePropertyWidget
  : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqDisplaySizedImplicitPlanePropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqDisplaySizedImplicitPlanePropertyWidget() override;

  /**
   * Overridden to update the DrawPlane state.
   */
  void apply() override;
  void reset() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Slots used to toggle the visibility of the translucent plane.
   */
  void showPlane() { this->setDrawPlane(true); }
  void hidePlane() { this->setDrawPlane(false); }
  void setDrawPlane(bool val);

  /**
   * Set draw outline and intersection edges.
   */
  void setDrawOutlineAndIntersectionEdges(bool val);

  /**
   * Set the widget normal to be along the X axis.
   */
  void useXNormal() { this->setNormal(1, 0, 0); }

  /**
   * Set the widget normal to be along the Y axis.
   */
  void useYNormal() { this->setNormal(0, 1, 0); }

  /**
   * Set the widget normal to be along the Z axis.
   */
  void useZNormal() { this->setNormal(0, 0, 1); }

  /**
   * Reset the disk-radius size to the default size.
   */
  void resetRadiusSize();

  /**
   * Update the widget's origin and bounds using current data bounds.
   */
  void resetToDataBounds();

  /**
   * Reset the camera to be down the plane normal.
   */
  void resetCameraToNormal();

  /**
   * Set the widget normal to be along the camera view direction.
   */
  void useCameraNormal();

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

private Q_SLOTS:
  void setOrigin(double x, double y, double z);
  void setNormal(double x, double y, double z);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqDisplaySizedImplicitPlanePropertyWidget)
};

#endif
