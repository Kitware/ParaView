/*=========================================================================

   Program: ParaView
   Module:    pqSelectionInspectorPanel.h

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

#ifndef _pqSelectionInspectorPanel_h
#define _pqSelectionInspectorPanel_h

#include "pqComponentsExport.h"
#include <QWidget>

class vtkObject;
class pqDataRepresentation;
class pqOutputPort;
class pqPipelineSource;
class pqProxy;
class pqSelectionManager;
class pqServer;
class pqServerManagerModelItem;
class pqView;
class QTreeWidgetItem;
class vtkSMClientDeliveryRepresentationProxy;
class vtkSMSourceProxy;
class vtkUnstructuredGrid;

/// pqSelectionInspectorPanel is a panel that shows shows the active selection.
/// It makes is possible for the user to view/change the active selection.
class PQCOMPONENTS_EXPORT pqSelectionInspectorPanel :
  public QWidget
{
  Q_OBJECT
  
public:
  pqSelectionInspectorPanel(QWidget* parent);
  ~pqSelectionInspectorPanel();

  /// Set the selection manager.
  void setSelectionManager(pqSelectionManager*);

public slots:
  /// Called when active server changes. We make the decision if process id
  /// needs to be shown for the server connection.
  void setServer(pqServer* server);

  /// Update the enabled state of the panel depending upon the current state of
  /// application.
  void updateEnabledState();

protected slots:
  /// Called when the active selection changes. The panel we show the details of
  /// the selection source input going into the pqOutputPort passed as an
  /// argument. Typically, this is connected to the
  /// pqSelectionManager::selectionChanged(pqOutputPort*) signal. 
  void select(pqOutputPort* opport, bool createNew=false);

  /// Called when the "Selection Type" combo-box is changed.
  void onSelectionTypeChanged(const QString&);

  /// Called when the SelectionManager is changed.
  void onSelectionManagerChanged(pqOutputPort* opport);

  /// Called when "Field Type" combo-box changes. This updates the enabled state
  /// of the "Containing Cells" combo-box, since that combo-box only makes sense
  /// for point selections.
  void onFieldTypeChanged(const QString&);

  /// Called when the user clicks "Create Selection" button.
  void createSelectionForCurrentObject();

  /// Called when the active view changes.
  void onActiveViewChanged(pqView*);

  void updatePointLabelMode(const QString&);
  void updateCellLabelMode(const QString&);
  void updateSelectionLabelEnableState();
  void updateSelectionPointLabelArrayName();
  void updateSelectionCellLabelArrayName();

  /// Called to update the IDs/GlobalIDs/Thresholds table.
  void newValue();
  void deleteValue();
  void deleteAllValues();

  /// Requests update on on the active view. 
  void updateRepresentationViews();

  /// Reqeusts render in all views the selection is shown.
  void updateAllSelectionViews();

  /// Called when user navigates beyond the end in the indices table widget. We
  /// add a new row to simplify editing.
  void onTableGrown(QTreeWidgetItem* item);

  /// Called when the current item in the "Indices" table changes. If composite
  /// tree is visible, we update the composite tree selection to match the
  /// current item.
  void onCurrentIndexChanged(QTreeWidgetItem* item);

  /// Update positions of point-handle widgets based on the selection locations.
  void updateLocationWidgets();

  /// Update selection locations based on point-handle widget positions.
  void updateLocationFromWidgets();

  /// Called when ShowFrustum checkbox is toggled.
  void updateFrustum();

  /// Called to update the types of selections available.
  void updateSelectionTypesAvailable();

  /// Called when selection color is changed, we save the selection color in
  /// settings so that it's preserved across sessions.
  void onSelectionColorChanged(const QColor& color);

  void forceLabelGlobalId(vtkObject* caller);
protected:
  /// Sets up the GUI by created default signal/slot bindings etc.
  void setupGUI();

  /// Sets up the links for the tab showing the details for an ID selection.
  void setupIDSelectionGUI();

  /// Sets up the links for the tab showing the defatils for a Global ID
  /// selection.
  void setupGlobalIDSelectionGUI();

  /// Sets up the GUI for the Locations selection.
  void setupLocationsSelectionGUI();

  /// Sets up the GUI for the Blocks selection.
  void setupBlockSelectionGUI();

  void setupSelectionLabelGUI();

  /// Sets up property links between the selection source proxy and the GUI.
  void updateSelectionGUI();

  /// Sets up the property links between the "Display Style" group and the
  /// selection representation proxy.
  void updateDisplayStyleGUI();

  void setupFrustumSelectionGUI();

  void setupThresholdSelectionGUI();
  void updateThreholdDataArrays();

  /// Create a new selection source for the current output port if
  /// * no selection source present
  /// * selection source type is not same as the "Selection Type" combo
  void createNewSelectionSourceIfNeeded();

  /// This returns the content type given the "Selection Type". 
  int getContentType() const;

  void removeWidgetsFromView();
  void addWidgetsToView();
  void allocateWidgets(unsigned int numWidgets);

  void updateFrustumInternal(bool);
  
  /// Called to update the types of selections available.
  void updateSelectionTypesAvailable(pqOutputPort* opport);

  /// Returns true if the port has GlobalIDs
  bool hasGlobalIDs(pqOutputPort*);

  void selectGlobalIdsIfPossible(pqOutputPort* opport, bool forceGlobalIds, bool createNew);
  void setGlobalIDs();
private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
