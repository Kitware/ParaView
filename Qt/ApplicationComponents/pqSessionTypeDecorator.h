/*=========================================================================

   Program: ParaView
   Module:  pqSessionTypeDecorator.h

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

  //@{
  /**
  * Methods overridden from pqPropertyWidget.
  */
  bool canShowWidget(bool show_advanced) const override;
  bool enableWidget() const override;
  //@}

private:
  Q_DISABLE_COPY(pqSessionTypeDecorator);

  bool IsVisible;
  bool IsEnabled;
};

#endif
