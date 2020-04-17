/*=========================================================================

   Program: ParaView
   Module:    pqRepresentation.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef _pqRepresentation_h
#define _pqRepresentation_h

#include "pqProxy.h"
#include <QPair>

class pqView;
class pqServer;
class vtkSMViewProxy;

/**
* This is PQ representation for a single representation.
* This class provides API for the Qt layer to access representations.
*/

class PQCORE_EXPORT pqRepresentation : public pqProxy
{
  Q_OBJECT
public:
  // Constructor.
  // \c group :- smgroup in which the proxy has been registered.
  // \c name  :- smname as which the proxy has been registered.
  // \c repr  :- the representation proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqRepresentation(const QString& group, const QString& name, vtkSMProxy* repr, pqServer* server,
    QObject* parent = NULL);
  ~pqRepresentation() override;

  /**
  * Returns if the status of the visbility property of this display.
  * Note that for a display to be visible in a view,
  * it must be added to that view as well as
  * visibility must be set to 1.
  */
  virtual bool isVisible() const;

  /**
  * Set the visibility. Note that this affects the visibility of the
  * display in the view it has been added to, if any. This method does not
  * call a re-render on the view, caller must call that explicitly.
  */
  virtual void setVisible(bool visible);

  /**
  * Returns the view to which this representation has been added, if any.
  */
  pqView* getView() const;

  /**
  * Returns the view proxy to which this representation has been added, if
  * any.
  */
  vtkSMViewProxy* getViewProxy() const;

public Q_SLOTS:

  /**
  * Renders the view to which this representation has been added if any.
  * If \c force is true, then the render is triggered immediately, otherwise,
  * it will be called on idle.
  */
  void renderView(bool force);

  /**
  * Simply calls renderView(false);
  */
  void renderViewEventually() { this->renderView(false); }

Q_SIGNALS:
  /**
  * Fired when the visibility property of the underlying display changes.
  * It must be noted that this is fired on the property change, the property
  * is not pushed yet, hence the visibility of the underlying VTK prop
  * hasn't changed.
  */
  void visibilityChanged(bool visible);

  /**
  * Fired whenever Update() is called on the underlying display proxy.
  */
  void updated();
protected Q_SLOTS:
  /**
  * called when the display visibility property changes.
  */
  virtual void onVisibilityChanged();

protected:
  friend class pqView;

  /**
  * Called by pqView when this representation gets added to / removed from the
  * view.
  */
  virtual void setView(pqView*);

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
