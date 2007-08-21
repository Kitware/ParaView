/*=========================================================================

   Program: ParaView
   Module:    pqSelectionInspectorPanel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#ifndef _pqSelectionInspectorPanel_h
#define _pqSelectionInspectorPanel_h

#include "pqComponentsExport.h"
#include <QWidget>

class pqDataRepresentation;
class pqPipelineSource;
class pqRubberBandHelper;
class pqSelectionManager;
class pqProxy;
class pqServer;
class pqServerManagerModelItem;
class vtkSMClientDeliveryRepresentationProxy;
class vtkSMSourceProxy;
class vtkUnstructuredGrid;

/// pqSelectionInspectorPanel has dual role:
/// \li showing the data from the active selection
/// \li showing the data from a "ExtractCellSelection" 
//      or "ExtractPointSelection" filter.
class PQCOMPONENTS_EXPORT pqSelectionInspectorPanel :
  public QWidget
{
  Q_OBJECT
  
public:
  pqSelectionInspectorPanel(QWidget* parent);
  ~pqSelectionInspectorPanel();

  /// Set the selection manager. The selection manager is used to
  /// obtain the current user defined cell selection.
  void setRubberBandHelper(pqRubberBandHelper* helper);

signals:

public slots:
  /// Called when user creates a new surface selection (or old 
  /// surface selection is cleared).
  void onSelectionChanged();

protected:

  void setupGUI();

  void setupSelelectionLabelGUI();
  void updateSelectionLabelModes();

  void setupSurfaceSelectionGUI();
  void updateSurfaceSelectionIDRanges();
  void updateSurfaceInformationAndDomains();

  void setupFrustumSelectionGUI();

  void setupThresholdSelectionGUI();
  void updateThreholdDataArrays();

  void updateSelectionRepGUI();
  void updateSelectionSourceGUI();

protected slots:

  void updatePointLabelMode(const QString&);
  void updateCellLabelMode(const QString&);
  void updateSelectionLabelEnableState();
  void updateSelectionPointLabelArrayName();
  void updateSelectionCellLabelArrayName();

  /// Deletes selected elements.
  void deleteSelectedSurfaceSelection();

  /// Deletes all elements.
  void deleteAllSurfaceSelection();

  // Adds a new value.
  void newValueSurfaceSelection();

  void addThresholds();
  void deleteSelectedThresholds();
  void deleteAllThresholds();
  void upperThresholdChanged(double);
  void lowerThresholdChanged(double);

  void updateSurfaceIDConnections();
  void updateSelectionFieldType(const QString&);
  void updateSelectionContentType(const QString&);

  void updateSurfaceSelectionView();
  void updateSelectionSource();

  virtual void onSelectionModeChanged(int mode);
  virtual void onSelectionContentTypeChanged();
  virtual void onActiveViewChanged();
  /// Requests update on all views the
  /// Representation is visible in.
  virtual void updateRepresentationViews();

  virtual void updateAllSelectionViews();

private:
  /// Set the display whose properties we want to edit.
  void setRepresentation(pqDataRepresentation* repr);
  void setInputSource(pqPipelineSource* input, int portnum);
  void setSelectionSource(vtkSMSourceProxy* source);
  void setEmptySelectionSource();
  void setSelectionManager(pqSelectionManager*);
  
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
