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

// qt includes
#include <QDockWidget>

// std includes
#include <unordered_map>

// forward declarations
class QAction;
class QLayout;

class pqProxy;
class pqPipelineSource;
class pqRepresentation;
class pqOutputPort;
class pqView;

class pqNodeEditorNode;
class pqNodeEditorEdge;
class pqNodeEditorScene;
class pqNodeEditorView;

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
  virtual ~pqNodeEditorWidget() = default;

public slots:
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

  //@{
  /**
   * Create/Remove the node corresponding to the given proxy
   */
  int createNodeForSource(pqPipelineSource* proxy);
  int createNodeForView(pqView* proxy);
  int removeNode(pqProxy* proxy);
  //@}

  /**
   * Given a consumer, set its input port @c idx to be connected with the
   * currently selected output ports.
   */
  int setInput(pqPipelineSource* consumer, int idx, bool clear);

  /**
   * Update style for the view nodes.
   */
  int updateActiveView();

  /**
   * Update style for ports and sources according to the current selection and active objects.
   */
  int updateActiveSourcesAndPorts();

  /**
   * Given a proxy, remove every edges connected to its input ports.
   */
  int removeIncomingEdges(pqProxy* proxy);

  /**
   * Rebuild every input edge of a given source proxy.
   */
  int updatePipelineEdges(pqPipelineSource* consumer);

  /**
   * Sets the style of ports.
   */
  int updatePortStyles();

  /**
   * Rebuild every input edges of a given view proxy.
   */
  int updateVisibilityEdges(pqView* proxy);

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

protected:
  pqNodeEditorNode* createNode(pqProxy* proxy);

  int initializeActions();
  int createToolbar(QLayout* layout);
  int attachServerManagerListeners();

private:
  pqNodeEditorScene* scene;
  pqNodeEditorView* view;

  bool autoUpdateLayout{ true };
  bool showViewNodes{ true };
  QAction* actionZoom;
  QAction* actionLayout;
  QAction* actionApply;
  QAction* actionReset;
  QAction* actionAutoLayout;
  QAction* actionCycleNodeVerbosity;
  QAction* actionToggleViewNodeVisibility;

  /**
   *  The node registry stores a node for each source/filter/view proxy
   *  The key is the global identifier of the node proxy.
   */
  std::unordered_map<int, pqNodeEditorNode*> nodeRegistry;

  /**
   *  The edge registry stores all incoming edges of a node.
   *  The key is the global identifier of the node proxy.
   */
  std::unordered_map<int, std::vector<pqNodeEditorEdge*>> edgeRegistry;
};

#endif // pqNodeEditorWidget_h
