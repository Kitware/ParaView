/*=========================================================================

   Program: ParaView
   Module:  pqAnimationShortcutDecorator.h

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
#ifndef pqAnimationShortcutDecorator_h
#define pqAnimationShortcutDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"

class pqAnimationShortcutWidget;
class vtkSMProxy;
class vtkSMProperty;

/**
 * A default decorator to add a pqAnimationShortcutWidget on property widgets
 * from a vtkSMSourceProxy if it is not a vtkSMRepresentationProxy,
 * and if the property is a vector property of a single elements
 * that has a range or a scalar range defined and is animateable.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnimationShortcutDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  /**
  * Constructor that modify the widget if conditions are met.
  */
  pqAnimationShortcutDecorator(pqPropertyWidget* parent);
  virtual ~pqAnimationShortcutDecorator();

  /**
   * Return true if the widget is considered valid by this decorator
   */
  static bool accept(pqPropertyWidget* widget);

protected Q_SLOTS:
  /**
   * Called when general settings has changed
   * to hide/show the widget if necessary
   */
  virtual void generalSettingsChanged();

private:
  Q_DISABLE_COPY(pqAnimationShortcutDecorator);

  pqAnimationShortcutWidget* Widget;
};

#endif
