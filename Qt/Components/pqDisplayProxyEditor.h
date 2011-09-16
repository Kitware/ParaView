/*=========================================================================

   Program: ParaView
   Module:    pqDisplayProxyEditor.h

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
#ifndef _pqDisplayProxyEditor_h
#define _pqDisplayProxyEditor_h

#include <QWidget>
#include <QList>
#include <QVariant>
#include "pqDisplayPanel.h"

class pqDisplayProxyEditorInternal;
class pqPipelineRepresentation;

/// Widget which provides an editor for the properties of a display.
class PQCOMPONENTS_EXPORT pqDisplayProxyEditor : public pqDisplayPanel
{
  Q_OBJECT

  // property adaptor for specular lighting
  Q_PROPERTY(QVariant specularColor READ specularColor
                                    WRITE setSpecularColor)
public:
  /// constructor
  pqDisplayProxyEditor(pqPipelineRepresentation* display, QWidget* p = NULL);
  /// destructor
  ~pqDisplayProxyEditor();

  /// TODO: get rid of this function once the server manager can
  /// inform us of display property changes
  void reloadGUI();

  /// When set to true (default) scalar coloring will result in disabling of the
  /// specular GUI.
  void setDisableSpecularOnScalarColoring(bool flag)
    { this->DisableSpecularOnScalarColoring = flag; }

signals:
  void specularColorChanged();

protected slots:
  /// internally used to update the graphics window when a property changes
  void openColorMapEditor();
  void rescaleToDataRange();
  void zoomToData();
  void updateEnableState();
  virtual void editCubeAxes();
  virtual void cubeAxesVisibilityChanged();
  void sliceDirectionChanged();
  void selectedMapperChanged();
  void volumeBlockSelected();
  void setSolidColor(const QColor& color);
  void setBackfaceSolidColor(const QColor& color);
  void setAutoAdjustSampleDistances(bool flag);

  void beginUndoSet(const QString&);
  void endUndoSet();
protected:

  /// Set the display whose properties we want to edit.
  virtual void setRepresentation(pqPipelineRepresentation* display);
  void updateSelectionLabelModes();

  pqDisplayProxyEditorInternal* Internal;
  void setupGUIConnections();

  QVariant specularColor() const;
  void setSpecularColor(QVariant);

  bool isCubeAxesVisible();


  bool DisableSpecularOnScalarColoring;
private:
  bool DisableSlots;
};

#endif

