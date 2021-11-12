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

#ifndef pqNodeEditorNode_h
#define pqNodeEditorNode_h

#include <QGraphicsItem>

class pqNodeEditorLabel;
class pqNodeEditorPort;
class pqPipelineSource;
class pqProxy;
class pqProxyWidget;
class pqView;
class QGraphicsScene;
class QGraphicsSceneMous;

/**
 * Every instance of this class corresponds to a node representing either a source,
 * a filter or a render view. They have severals input and output pqNodeEditorPort,
 * allowing them to connect to each other.
 *
 * See below for an ASCII art of the coordinate system for this item (useful to
 * understand how are implemented @c paint and @c boundingRect functions for example).
 * `X` is the position (0,0), `═` signs are the border, `O` is the position of a port.
 *
 * ╔═══════════════╗
 * ║X-------------+║
 * ║| (headline)  |║   <- PORT_PADDING y dimension |
 * ║O    Label    O║   <- PORT_HEIGHT  y dimension | <= sum := pqNodeEditorNode::headlineHeight
 * ║|             |║   <- PORT_PADDING y dimension |
 * ║+-------------+║
 * ║|             |║
 * ║|  Properties |║
 * ║|    Panel    |║
 * ║|             |║
 * ║+-------------+║
 * ╚═══════════════╝
 *
 * @sa
 * pqNodeEditorPort
 * pqProxyWidget
 */
class pqNodeEditorNode
  : public QObject
  , public QGraphicsItem
{
  Q_OBJECT
  Q_INTERFACES(QGraphicsItem)

public:
  /**
   * Enum for the verbosity style of the nodes in the node editor scene.
   * EMPTY : no properties displayed
   * NORMAL : default properties displayed
   * ADVANCED : default and advanced properties displayed
   */
  enum class Verbosity : int
  {
    EMPTY = 0,
    NORMAL,
    ADVANCED
  };

  /**
   * Enum for the outline style of the nodes in the node editor scene.
   * NORMAL : node is not selected
   * SELECTED_FILTER : node represent a filter and is selected
   * SELECTED_VIEW : node represent a view and is selected
   */
  enum class OutlineStyle : int
  {
    NORMAL = 0,
    SELECTED_FILTER,
    SELECTED_VIEW
  };

  /**
   * Enum for the background style of the nodes in the node editor scene.
   * NORMAL : node has not been modified since last Apply
   * DIRTY : node properties has been modified
   */
  enum class BackgroundStyle : int
  {
    NORMAL = 0,
    DIRTY
  };

  /**
   * Static method to cycle through default verbosity levels in deceasing order.
   */
  static pqNodeEditorNode::Verbosity CycleDefaultVerbosity()
  {
    return pqNodeEditorNode::DefaultNodeVerbosity = static_cast<pqNodeEditorNode::Verbosity>(
             (static_cast<int>(pqNodeEditorNode::DefaultNodeVerbosity) + 2) % 3);
  };

  /**
   * Creates a node for the given pqPipelineSource instance. This will also create the input/ouput
   * ports on the left/right of the node.
   */
  pqNodeEditorNode(
    QGraphicsScene* scene, pqPipelineSource* source, QGraphicsItem* parent = nullptr);

  /**
   *  Create a node and its input port representing the given @c view.
   */
  pqNodeEditorNode(QGraphicsScene* scene, pqView* view, QGraphicsItem* parent = nullptr);

  /**
   * Remove the node from the scene it has been added to.
   */
  ~pqNodeEditorNode() override;

  /**
   *  Get corresponding pqProxy of the node.
   */
  pqProxy* getProxy() { return this->proxy; }

  /**
   *  Get input ports of the node.
   */
  std::vector<pqNodeEditorPort*>& getInputPorts() { return this->iPorts; }

  /**
   * Get output ports of the node.
   */
  std::vector<pqNodeEditorPort*>& getOutputPorts() { return this->oPorts; }

  /**
   * Get widget container of the node.
   */
  pqProxyWidget* getProxyProperties() { return this->proxyProperties; }

  /**
   *  Get the label of the node.
   */
  pqNodeEditorLabel* getLabel() { return this->label; }

  //@{
  /**
   * Get/Set the verbosity level of the node, that is the amount of properties
   * displayed in the node. It can be either empty, simple or advandeced (every properties).
   * Update the style accordingly.
   */
  void setVerbosity(Verbosity v);
  Verbosity getVerbosity() { return this->verbosity; };
  //@}

  /**
   * Increment verbosity of the displayed properties of the node. If we try to increment
   * past the last level of verbosity we go back to the first level.
   * Update the style accordingly and return the new verbosity level.
   */
  void incrementVerbosity();

  //@{
  /**
   * Get/Set the type of the node. It can be either NORMAL (unselected), SELECTED_FILTER
   * (for the active source) or SELECTED_VIEW (for the active view). Update the style accordingly.
   */
  void setOutlineStyle(OutlineStyle style);
  OutlineStyle getOutlineStyle() { return this->outlineStyle; };
  //@}

  //@{
  /**
   * Get/Set the background style for this node, wether the filter is dirty or not.
   * Update the style accordingly.
   * 0: BackgroundStyle::NORMAL, 1: BackgroundStyle::DIRTY
   */
  void setBackgroundStyle(BackgroundStyle style);
  BackgroundStyle getBackgroundStyle() { return this->backgroundStyle; };
  //@}

  /**
   * Get the bounding box of the node, which includes the border width and the label.
   */
  QRectF boundingRect() const override;

Q_SIGNALS:
  void nodeResized();
  void nodeMoved();

protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  /**
   *  Update the size of the node to fit its contents.
   */
  int updateSize();

private:
  /**
   * Internal constructor used by the public ones for initializing the node regardless
   * of what the proxy represents. Initialize things such as the dimensions, the label, etc.
   */
  pqNodeEditorNode(QGraphicsScene* scene, pqProxy* proxy, QGraphicsItem* parent = nullptr);

  QGraphicsScene* scene;
  pqProxy* proxy;
  pqProxyWidget* proxyProperties;
  QWidget* widgetContainer;
  pqNodeEditorLabel* label;

  std::vector<pqNodeEditorPort*> iPorts;
  std::vector<pqNodeEditorPort*> oPorts;

  OutlineStyle outlineStyle{ OutlineStyle::NORMAL };
  BackgroundStyle backgroundStyle{ BackgroundStyle::NORMAL };
  Verbosity verbosity{ Verbosity::EMPTY };

  // Height of the headline of the node.
  // Should be computed in the constructor and never assigned again.
  int headlineHeight{ 0 };
  int labelHeight{ 0 };

  /**
   * Static property that controls the verbosity of nodes upon creation.
   */
  static pqNodeEditorNode::Verbosity DefaultNodeVerbosity;
};

#endif // pqNodeEditorNode_h
