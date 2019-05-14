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
#ifndef pqIntegrationModelSurfaceHelperWidget_h
#define pqIntegrationModelSurfaceHelperWidget_h

#include "pqIntegrationModelHelperWidget.h"

class vtkStringArray;
class vtkPVDataInformation;

/// Class to represent ArraysToGenerate property
/// with the Lagrangian Surface Helper widget,
/// It manually creates the widgets and updates the properties
/// accordingly
class pqIntegrationModelSurfaceHelperWidget : public pqIntegrationModelHelperWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> arrayToGenerate READ arrayToGenerate WRITE setArrayToGenerate)
  typedef pqIntegrationModelHelperWidget Superclass;

public:
  pqIntegrationModelSurfaceHelperWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = 0);
  ~pqIntegrationModelSurfaceHelperWidget() override = default;

  QList<QVariant> arrayToGenerate() const;

public slots:
  void setArrayToGenerate(const QList<QVariant>&);

signals:
  void arrayToGenerateChanged();

protected slots:
  /// Create/Reset the widget
  void resetWidget() override;

protected:
  /// Recursive method to fill leaf names from a potentially composite data information
  /// to a flat string array
  static vtkStringArray* fillLeafNames(
    vtkPVDataInformation* info, QString baseName, vtkStringArray* names);

private:
  Q_DISABLE_COPY(pqIntegrationModelSurfaceHelperWidget);

  /// Non virtual resetWidget method
  void resetSurfaceWidget(bool force);
};

#endif
