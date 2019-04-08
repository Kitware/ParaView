/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef pqIntegrationModelSeedHelperWidget_h
#define pqIntegrationModelSeedHelperWidget_h

#include "pqIntegrationModelHelperWidget.h"

class vtkSMInputProperty;

/// Class to represent ArraysToGenerate property
/// with the Lagrangian Seed Helper widget,
/// It manually creates the widgets and updates the properties
/// accordingly
class pqIntegrationModelSeedHelperWidget : public pqIntegrationModelHelperWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> arrayToGenerate READ arrayToGenerate WRITE setArrayToGenerate)
  typedef pqIntegrationModelHelperWidget Superclass;

public:
  pqIntegrationModelSeedHelperWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = 0);
  ~pqIntegrationModelSeedHelperWidget() override = default;

  QList<QVariant> arrayToGenerate() const;

public slots:
  void setArrayToGenerate(const QList<QVariant>&);

signals:
  void arrayToGenerateChanged();

protected slots:
  /// Create/Reset the widget
  void resetWidget() override;
  void forceResetSeedWidget();

  /// Update the enable state of certain widgets in this widget
  void updateEnabledState();

private:
  Q_DISABLE_COPY(pqIntegrationModelSeedHelperWidget);

  /// Non-virtual resetWidget method
  void resetSeedWidget(bool force);

  vtkSMInputProperty* FlowInputProperty;
};

#endif
