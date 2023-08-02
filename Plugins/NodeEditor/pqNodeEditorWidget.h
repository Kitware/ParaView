// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqNodeEditorWidget_h
#define pqNodeEditorWidget_h

#include <QDockWidget>

#include "vtkType.h"

#include <unordered_map>

class pqNodeEditorAnnotationItem;
class pqNodeEditorApplyBehavior;
class pqNodeEditorEdge;
class pqNodeEditorNode;
class pqNodeEditorScene;
class pqNodeEditorView;
class pqProxy;
class pqPipelineFilter;
class pqPipelineSource;
class pqOutputPort;
class pqView;
class pqRepresentation;

class QAction;
class QCheckBox;
class QLayout;

/**
 * This is the root widget of the node editor that can be docked in ParaView.
 * It contains the canvas where nodes are drawn, and a toolbar to access severals tools.
 */
class pqNodeEditorWidget : public QDockWidget
{
  Q_OBJECT

public:
  pqNodeEditorWidget(QWidget* parent = nullptr);
  pqNodeEditorWidget(const QString& title, QWidget* parent = nullptr);
  ~pqNodeEditorWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Update ParaView pipeline and views
   */
  int apply();

  /**
   * Reset the properties of all nodes to there previous Apply values.
   */
  int reset();

  /**
   * Auto zoom to have all the nodes and edges in the node editor scene viewport
   */
  int zoom();

  /**
   * Update style for the view nodes.
   */
  int updateActiveView(pqView* view = nullptr);

  /**
   * Update style for ports and sources according to the current selection and active objects.
   */
  int updateActiveSourcesAndPorts();

  /**
   * Toggle the visibility of the given output port int the active view.
   * if exclusive == true then only the given port will be shown.
   */
  int toggleInActiveView(pqOutputPort* port, bool exclusive = false);

  /**
   * Hide every actors in the active view
   *
   */
  int hideAllInActiveView();

  /**
   * Cycle the node verbosity in incrasing order
   */
  int cycleNodeVerbosity();

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  ///@{
  /**
   * Create/Remove the node corresponding to the given proxy
   */
  int createNodeForSource(pqPipelineSource* proxy);
  int createNodeForView(pqView* proxy);
  int createNodeForRepresentation(pqRepresentation* repr);
  int removeNode(pqProxy* proxy);
  ///@}

  /**
   * Show/Hide view and representation nodes and edges according to `showViewNodes` and
   * the visibility of the representation.
   */
  void toggleViewNodesVisibility();

  /**
   * Given a consumer, set its input port @c idx to be connected with the
   * currently selected output ports.
   */
  int setInput(pqPipelineSource* consumer, int idx, bool clear);

  /**
   * Given a proxy, remove every edges connected to its input ports. Only affects the UI.
   */
  int removeIncomingEdges(pqProxy* proxy);

  /**
   * Rebuild every input edge of a given source proxy.
   */
  int updatePipelineEdges(pqPipelineFilter* consumer);

  ///@{
  /**
   * Import/Export layout from/as a Qt settings file. This is called whenver a state file is
   * loaded/saved. Use this->processedStateFile to determine where to save the layout.
   */
  void exportLayout();
  void importLayout();
  ///@}

  /**
   * Handle creation and delection of annotation items. Called when the view catch the
   * related user input.
   *
   * @param del if true then will delete all selected annotation items. Id false
   * it will create a new annotation item that will contain currently selected pipeline items.
   */
  void annotateNodes(bool del);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Register a specific node in the scene and other internals structures.
   */
  void registerNode(pqNodeEditorNode* node, vtkIdType id);

  int initializeActions();
  int initializeSignals();
  int createToolbar(QLayout* layout);
  int attachServerManagerListeners();

  /**
   * Set the visibility of a specified port. It implements 3 behaviors :
   * vis == 0: hide the port
   * vis  > 0: show the port
   * vis  < 0: toggle the visibility of the port
   */
  void setPortVisibility(pqOutputPort* port, pqView* view, int vis);

  /**
   * Construct the absolute path to the layout file using this->processedStateFile
   * Returns an empty string if processedStateFile is empty.
   * This function assume processedStateFile is a valid path (even though the actual file may not
   * exist)
   */
  QString constructLayoutFilename() const;

private:
  pqNodeEditorScene* scene;
  pqNodeEditorView* view;

  // set using signals when state files are loaded / saved
  QString processedStateFile;

  bool autoUpdateLayout{ true };
  bool showViewNodes{ true };
  QAction* actionZoom;
  QAction* actionLayout;
  QAction* actionApply;
  QAction* actionReset;
  QAction* actionAutoLayout;
  QAction* actionCycleNodeVerbosity;
  QAction* actionToggleViewNodeVisibility;
  pqNodeEditorApplyBehavior* applyBehavior;

  QCheckBox* autoLayoutCheckbox;

  /**
   *  The node registry stores a node for each source/filter/view proxy
   *  The key is the global identifier of the node proxy.
   */
  std::unordered_map<vtkIdType, pqNodeEditorNode*> nodeRegistry;

  /**
   *  The edge registry stores all incoming edges of a node.
   *  The key is the global identifier of the node proxy.
   */
  std::unordered_map<vtkIdType, std::vector<pqNodeEditorEdge*>> edgeRegistry;

  std::vector<pqNodeEditorAnnotationItem*> annotationRegistry;
};

#endif // pqNodeEditorWidget_h
