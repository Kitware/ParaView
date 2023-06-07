/*=========================================================================

  Program: ParaView
  Module: pqCustomViewpointsDefaultController.h

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
#ifndef pqCustomViewpointsDefaultController_h
#define pqCustomViewpointsDefaultController_h

#include "pqApplicationComponentsModule.h"

#include "pqCustomViewpointsController.h"

/**
 * @brief Default custom viewpoints controller
 *
 * This controller controls the global desktop view custom viewpointss
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomViewpointsDefaultController
  : public pqCustomViewpointsController
{
  Q_OBJECT
  typedef pqCustomViewpointsController Superclass;

public:
  pqCustomViewpointsDefaultController(QObject* parent = nullptr);

  ~pqCustomViewpointsDefaultController() override = default;

  /**
   * @brief Get tooltips of all viewpoints
   * @return QStringList of tooltips
   * @see pqCameraDialog::CustomViewpointToolTips
   */
  QStringList getCustomViewpointToolTips() override;

  /**
   * @brief Called when configure button is pressed
   * @see pqCameraDialog::configureCustomViewpoints
   */
  void configureCustomViewpoints() override;

  /**
   * @brief Set the specified viewpoint entry to current viewpoint
   * @see pqCameraDialog::setToCurrentViewpoint
   */
  void setToCurrentViewpoint(int index) override;

  /**
   * @brief Move camera to match specified viewpoint entry
   * @see pqCameraDialog::applyCustomViewpoint
   */
  void applyCustomViewpoint(int index) override;

  /**
   * @brief Remove a custom viewpoint entry
   * @see pqCameraDialog::deleteCustomViewpoint
   */
  void deleteCustomViewpoint(int index) override;

  /**
   * @brief Save current viewpoint in a new viewpoint entry
   * @see pqCameraDialog::addCurrentViewpointToCustomViewpoints
   */
  void addCurrentViewpointToCustomViewpoints() override;
};

#endif
