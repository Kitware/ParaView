/*=========================================================================

   Program: ParaView
   Module:    pqBoxChartView.h

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

========================================================================*/
#ifndef pqBoxChartView_h
#define pqBoxChartView_h

#include "pqContextView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;

/**
* Bar chart view
*/
class PQCORE_EXPORT pqBoxChartView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  static QString chartViewType() { return "BoxChartView"; }

  pqBoxChartView(const QString& group, const QString& name, vtkSMContextViewProxy* viewModule,
    pqServer* server, QObject* parent = nullptr);

  ~pqBoxChartView() override;

Q_SIGNALS:
  /**
  * Fired when the currently shown representation changes. \c repr may be
  * nullptr.
  */
  void showing(pqDataRepresentation* repr);

public Q_SLOTS:
  /**
  * Called when a new repr is added.
  */
  void onAddRepresentation(pqRepresentation*);
  void onRemoveRepresentation(pqRepresentation*);

protected Q_SLOTS:
  /**
  * Called to ensure that at most 1 repr is visible at a time.
  */
  void updateRepresentationVisibility(pqRepresentation* repr, bool visible);

private:
  Q_DISABLE_COPY(pqBoxChartView)
};

#endif
