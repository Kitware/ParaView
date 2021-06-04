/*=========================================================================
Copyright (c) 2021, Jonas Lukasczyk
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

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

/// This is the root widget of the node editor that can be docked in ParaView.
/// It currently only contains the node editor canvas, but in the future one
/// can also add a toolbar.
class pqNodeEditorWidget : public QDockWidget
{
  Q_OBJECT

public:
  pqNodeEditorWidget(QWidget* parent = nullptr);
  pqNodeEditorWidget(const QString& title, QWidget* parent = nullptr);
  ~pqNodeEditorWidget();

protected:
  pqNodeEditorNode* createNode(pqProxy* proxy);

  int initializeActions();
  int createToolbar(QLayout* layout);
  int attachServerManagerListeners();

public slots:
  int apply();
  int reset();
  int zoom();
  int layout();

  int createNodeForSource(pqPipelineSource* proxy);
  int createNodeForView(pqView* proxy);
  int removeNode(pqProxy* proxy);

  int setInput(pqPipelineSource* consumer, int idx, bool clear);

  int updateActiveView();
  int updateActiveSourcesAndPorts();

  int removeIncomingEdges(pqProxy* proxy);
  int updatePipelineEdges(pqPipelineSource* consumer);
  int updateVisibilityEdges(pqView* proxy);

  int toggleInActiveView(pqOutputPort* port);
  int hideAllInActiveView();

  int collapseAllNodes();

private:
  pqNodeEditorScene* scene;
  pqNodeEditorView* view;

  bool autoUpdateLayout{ true };
  QAction* actionZoom;
  QAction* actionLayout;
  QAction* actionApply;
  QAction* actionReset;
  QAction* actionAutoLayout;
  QAction* actionCollapseAllNodes;

  /// The node registry stores a node for each source/filter/view proxy
  /// The key is the global identifier of the node proxy.
  std::unordered_map<int, pqNodeEditorNode*> nodeRegistry;

  /// The edge registry stores all incoming edges of a node.
  /// The key is the global identifier of the node proxy.
  std::unordered_map<int, std::vector<pqNodeEditorEdge*> > edgeRegistry;
};

#endif // pqNodeEditorWidget_h
