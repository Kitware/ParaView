/*=========================================================================

  Program:   ParaView
  Plugin:    NodeEditor

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  ParaViewPluginsNodeEditor - BSD 3-Clause License - Copyright (C) 2021 Jonas Lukasczyk

  See the Copyright.txt file provided
  with ParaViewPluginsNodeEditor for license information.
-------------------------------------------------------------------------*/

#ifndef pqNodeEditorWidget_h
#define pqNodeEditorWidget_h

#include <QDockWidget>

#include "vtkType.h"

#include <unordered_map>

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
  int updateActiveView();

  /**
   * Update style for ports and sources according to the current selection and active objects.
   */
  int updateActiveSourcesAndPorts();

  /**
   * Sets the style of ports.
   */
  int updatePortStyles();

  /**
   * Toggle the visibility of the given output port int the active view.
   */
  int toggleInActiveView(pqOutputPort* port);

  /**
   * Hide every actors in the active view
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
  int removeNode(pqProxy* proxy);
  ///@}

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
   * Rebuild every input edges of a given view proxy.
   */
  int updateVisibilityEdges(pqView* proxy);

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

protected:
  void initializeNode(pqNodeEditorNode* node, vtkIdType id);

  int initializeActions();
  int initializeSignals();
  int createToolbar(QLayout* layout);
  int attachServerManagerListeners();

  // Construct the absolute path to the layout file using this->processedStateFile
  // Returns an empty string if processedStateFile is empty.
  // This function assume processedStateFile is a valid path (even though the actual file may not
  // exist)
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
};

#endif // pqNodeEditorWidget_h
