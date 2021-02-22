/*=========================================================================

   Program: ParaView
   Module:    pqPipelineModel.h

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
/**
* \class pqPipelineModel
* \brief
*   The pqPipelineModel class is used to represent the pipeline graph
*   as a tree.
*
* The top of the hierarchy is the server objects. Each connected
* server is added to the root node. An object with no inputs is added
* as a child of its server. The outputs of an object are added as its
* children. Objects with multiple inputs present a problem.
*
* Instead of repeating information in the tree, the object with
* multiple inputs is moved to the server. All its outputs are treated
* normally and added as children. In place of the object with more
* than one input, a link object is placed in the child list of the
* input objects. The link item represents a connection instead of a
* pipeline object.
*/

#ifndef pqPipelineModel_h
#define pqPipelineModel_h

#include "pqComponentsModule.h" // For export macro
#include "pqView.h"             // For View

#include <QAbstractItemModel>
#include <QMap>     // For PixmapMap
#include <QPointer> // For View

class ModifiedLiveInsituLink;
class QFont;
class QPixmap;
class QString;
class pqExtractor;
class pqPipelineModelDataItem;
class pqPipelineModelInternal;
class pqPipelineSource;
class pqServer;
class pqServerManagerModel;
class pqServerManagerModelItem;
class vtkSession;

/**
* This class is the model for the PipelineLine browser tree view.
* pqServerManagerModel models the vtkSMProxyManager for the GUI. The
* vtkSMProxyManager maintains all proxies and hence it is difficult
* to detect/trasvers pipelines etc etc. The pqServerManagerModel
* provides a simplified view of the Server Manager. This class
* takes that simplified "model" and transforms it into hierarchical
* tables which can be represented by the Tree View.
*/
class PQCOMPONENTS_EXPORT pqPipelineModel : public QAbstractItemModel
{
  Q_OBJECT;

public:
  enum ItemType
  {
    Invalid = -1,
    Server = 0,
    Proxy,
    Port,
    Extractor,
    Link
  };

  enum ItemRole
  {
    AnnotationFilterRole = 33,
    SessionFilterRole = 34
  };

public:
  pqPipelineModel(QObject* parent = 0);

  /**
  * \brief
  *   Makes a copy of a pipeline model.
  * \param other The pipeline model to copy.
  * \param parent The parent object.
  */
  pqPipelineModel(const pqPipelineModel& other, QObject* parent = 0);

  /**
  * \brief
  *   Creates a pipeline model from a server manager model.
  * \param other Used to build a pipeline model.
  * \param parent The parent object.
  */
  pqPipelineModel(const pqServerManagerModel& other, QObject* parent = 0);

  ~pqPipelineModel() override;

  /**
  * \name QAbstractItemModel Methods
  */
  //@{
  /**
  * \brief
  *   Gets the number of rows for a given index.
  * \param parent The parent index.
  * \return
  *   The number of rows for the given index.
  */
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets the number of columns for a given index.
  * \param parent The parent index.
  * \return
  *   The number of columns for the given index.
  */
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets whether or not the given index has child items.
  * \param parent The parent index.
  * \return
  *   True if the given index has child items.
  */
  bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets a model index for a given location.
  * \param row The row number.
  * \param column The column number.
  * \param parent The parent index.
  * \return
  *   A model index for the given location.
  */
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets the parent for a given index.
  * \param index The model index.
  * \return
  *   A model index for the parent of the given index.
  */
  QModelIndex parent(const QModelIndex& index) const override;

  /**
  * \brief
  *   Gets the data for a given model index.
  * \param index The model index.
  * \param role The role to get data for.
  * \return
  *   The data for the given model index.
  */
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
  * \brief
  *  Sets the role data for the item at index to value. Returns
  *  true if successful; otherwise returns false.
  */
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

  /**
  * \brief
  *   Gets the flags for a given model index.
  *
  * The flags for an item indicate if it is enabled, editable, etc.
  *
  * \param index The model index.
  * \return
  *   The flags for the given model index.
  */
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  //@}

  /**
  * \name Object Mapping
  */
  //@{

  /**
  * Given the index, get the pqServerManagerModelItem it represents.
  * nullptr is returned for root or invalid index.
  */
  pqServerManagerModelItem* getItemFor(const QModelIndex&) const;

  QModelIndex getIndexFor(pqServerManagerModelItem* item) const;

  /**
  * \brief
  *   Gets the type for the given index.
  * \param index The model index to look up.
  * \return
  *   The type for the given index.
  */
  ItemType getTypeFor(const QModelIndex& index) const;
  //@}

  /**
  * \brief
 *   Gets whether or not the model indexes are editable.
 * \return
 *   True if the model indexes can be edited.
 */
  bool isEditable() const { return this->Editable; }

  /**
  * \brief
  *   Sets whether or not the model indexes are editable.
  * \param editable True if the model indexes can be edited.
  */
  void setEditable(bool editable) { this->Editable = editable; }

  /**
  * \brief
  *   Sets whether or not the given index is selectable.
  * \param index The model index.
  * \param selectable True if the index can be selected.
  */
  void setSelectable(const QModelIndex& index, bool selectable);

  /**
  * \brief
  *   Gets whether or not the given index is selectable.
  * \param index The model index.
  * \return
  *   True if the given index is selectable.
  */
  bool isSelectable(const QModelIndex& index) const;

  /**
  * \brief
  *   Sets whether of not an item subtree is selectable.
  * \param item The root of the subtree.
  * \param selectable True if the items can be selected.
  */
  void setSubtreeSelectable(pqServerManagerModelItem* item, bool selectable);

  /**
  * \brief
  *   Gets the next model index in the tree.
  * \param index The current index.
  * \param root An alternate root for walking a subtree.
  * \return
  *   An index to the next item in the tree or an invalid index
  *   when the end of the tree is reached.
  */
  QModelIndex getNextIndex(const QModelIndex index, const QModelIndex& root = QModelIndex()) const;

  /**
  * Provides access to the view.
  */
  pqView* view() const { return this->View; }

  /**
  * \brief
  *   Sets the font hint for modified items.
  * \param font The font to use for modified items.
  */
  void setModifiedFont(const QFont& font);

  /**
  * \brief
  *   Store the annotation key that will be used when
  *   "this->data( ... , pqPipelineMode::AnnotationFilterRole)"
  *   get called.
  * \param expectedAnnotation key that will be lookup inside the above code.
  */
  void enableFilterAnnotationKey(const QString& expectedAnnotation);

  /**
  * \brief
  *   Disable annotation key, so
  *   "this->data( ... , pqPipelineMode::AnnotationFilterRole)"
  *   will always return a QVariant("true")
  */
  void disableFilterAnnotationKey();

  /**
   * Set wether annotation filter should display matching or non matching sources.
   */
  void setAnnotationFilterMatching(bool matching);

  /**
  * \brief
  *   Store the session key that will be used when
  *   "this->data( ... , pqPipelineMode::SessionFilterRole)"
  *   get called.
  * \param session that will be lookup inside the above code.
  */
  void enableFilterSession(vtkSession* session);

  /**
  * \brief
  *   Disable annotation key, so
  *   "this->data( ... , pqPipelineMode::SessionFilterRole)"
  *   will always return a QVariant("true")
  */
  void disableFilterSession();

public Q_SLOTS:
  /**
  * Called when a new server connection is detected. Adds the connection to the
  * list.
  */
  void addServer(pqServer* server);

  /**
  * Called when a server connection is closed. Removes the server from the list.
  */
  void removeServer(pqServer* server);

  /**
  * Called when a new source/filter/bundle is registered.
  */
  void addSource(pqPipelineSource* source);

  /**
  * Called when a new source/filter/bundle is unregistered.
  */
  void removeSource(pqPipelineSource* source);

  /**
  * Called when new pipeline connection (between two pipeline objects)
  * is made.
  */
  void addConnection(pqPipelineSource* source, pqPipelineSource* sink, int);

  /**
  * Called when new pipeline connection (between two pipeline objects)
  * is broken.
  */
  void removeConnection(pqPipelineSource* source, pqPipelineSource* sink, int);

  //@{
  /**
   * Called to update extractor connections.
   */
  void addConnection(pqServerManagerModelItem* source, pqExtractor* sink);
  void removeConnection(pqServerManagerModelItem* source, pqExtractor* sink);
  //@}

  //@{
  /**
   * Add/remove extractor.
   */
  void addExtractor(pqExtractor*);
  void removeExtractor(pqExtractor*);
  //@}

  /**
  * Updates the icons in the current window column.
  * The current window column shows whether or not the source is
  * displayed in the current window. When the current window changes
  * the entire column needs to be updated.
  */
  void setView(pqView* module);

Q_SIGNALS:
  void firstChildAdded(const QModelIndex& index);

private Q_SLOTS:
  void onInsituConnectionInitiated(pqServer* server);

  void serverDataChanged();

  /**
  * called when visibility of the source may have changed.
  */
  void updateVisibility(pqPipelineSource*, ItemType type = Proxy);

  /**
  * provides a mechanism to delay updating of visibility while safely handling
  * the case where the pqPipelineSource itself gets deleted in the mean time.
  */
  void delayedUpdateVisibility(pqPipelineSource*);
  void delayedUpdateVisibilityTimeout();

  /**
  * called when the item's name changes.
  */
  void updateData(pqServerManagerModelItem*, ItemType type = Proxy);
  void updateDataServer(pqServer* server);

private:
  friend class pqPipelineModelDataItem;

  // Add an item as a child under the parent at the given index.
  // Note that this method does not actually change the underlying
  // pqServerManagerModel, it merely signals that such an addition
  // has taken place.
  void addChild(pqPipelineModelDataItem* parent, pqPipelineModelDataItem* child);

  // Remove a child item from under the parent.
  // Note that this method does not actually change the underlying
  // pqServerManagerModel, it merely signals that such an addition
  // has taken place.
  void removeChildFromParent(pqPipelineModelDataItem* child);

  // Returns the pqPipelineModelDataItem for the given pqServerManagerModelItem.
  pqPipelineModelDataItem* getDataItem(pqServerManagerModelItem* item,
    pqPipelineModelDataItem* subtreeRoot, ItemType type = Invalid) const;

  // called by pqPipelineModelDataItem to indicate that the data for the item
  // may have changed.
  void itemDataChanged(pqPipelineModelDataItem*);
  /**
  * used by the variant of setSubtreeSelectable() for recursion.
  */
  void setSubtreeSelectable(pqPipelineModelDataItem* item, bool selectable);

  QModelIndex getIndex(pqPipelineModelDataItem* item) const;

  /**
   * Check the PixmapMap contains a pixmap associated to the provided iconType.
   * Return true if yes.
   * If not it will try to load a new pixmap interpreting iconType as a Qt resource name
   * and add it to the map.
   * Return true if sucessful, false otherwise.
   */
  bool checkAndLoadPipelinePixmap(const QString& iconType);

private:
  pqPipelineModelInternal* Internal; ///< Stores the pipeline representation.
  QMap<QString, QPixmap> PixmapMap;  ///< Stores the item icons.
  QPointer<pqView> View;
  bool Editable;
  bool FilterAnnotationMatching;
  QString FilterRoleAnnotationKey;
  vtkSession* FilterRoleSession;
  ModifiedLiveInsituLink* LinkCallback;
  void constructor();

  friend class ModifiedLiveInsituLink;
};

#endif
