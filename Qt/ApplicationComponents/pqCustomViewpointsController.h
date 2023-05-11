/*=========================================================================

  Program: ParaView
  Module: pqCustomViewpointsController.h

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

#ifndef pqCustomViewpointsController_h
#define pqCustomViewpointsController_h

#include "pqApplicationComponentsModule.h"

#include <QObject>

class pqCustomViewpointsToolbar;

/**
 * @brief Base class for custom viewpoints controllers
 *
 * pqCustomViewpointsController is an abstract class that controls
 * the behaviour of a pqCustomViewpointsToolbar
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomViewpointsController : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCustomViewpointsController(QObject* parent = nullptr)
    : Superclass(parent)
  {
  }

  ~pqCustomViewpointsController() override = default;

  /**
   * @brief Set the toolbar controlled by `this`
   *
   * It will set `toolbar` as parent of `this`
   */
  void setToolbar(pqCustomViewpointsToolbar* toolbar);

  /**
   * @brief Get the toolbar controlled by `this`
   */
  pqCustomViewpointsToolbar* getToolbar() const { return this->Toolbar; }

  /**
   * @brief Get tooltips of all viewpoints
   * @return QStringList of tooltips
   */
  virtual QStringList getCustomViewpointToolTips() = 0;

  /**
   * @brief Called when configure button is pressed
   */
  virtual void configureCustomViewpoints() = 0;

  /**
   * @brief Set the specified viewpoint entry to current viewpoint
   */
  virtual void setToCurrentViewpoint(int index) = 0;

  /**
   * @brief Move camera to match specified viewpoint entry
   */
  virtual void applyCustomViewpoint(int index) = 0;

  /**
   * @brief Remove a custom viewpoint entry
   */
  virtual void deleteCustomViewpoint(int index) = 0;

  /**
   * @brief Save current viewpoint in a new viewpoint entry
   */
  virtual void addCurrentViewpointToCustomViewpoints() = 0;

private:
  pqCustomViewpointsToolbar* Toolbar = nullptr;
};

#endif
