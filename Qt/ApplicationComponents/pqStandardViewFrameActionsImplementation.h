// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqStandardViewFrameActionsImplementation_h
#define pqStandardViewFrameActionsImplementation_h

#include "pqApplicationComponentsModule.h" // needed for export macros
#include "pqViewFrameActionsInterface.h"
#include <QObject>  // needed for QObject
#include <QPointer> // needed for QPointer

class pqContextView;
class pqRenderView;
class pqSpreadSheetView;
class pqView;
class QAction;
class QActionGroup;
class QShortcut;
class QWidget;

/**
 * pqStandardViewFrameActionsImplementation implements
 * pqViewFrameActionsInterface to add the default view toolbar
 * buttons/actions used in ParaView for various types of views,
 * including chart views and render views.
 *
 * Toolbar buttons/actions can be added/removed in the XML for a view
 * using hints. For example,
 *
 * \verbatim
 * <Hints>
 *   <StandardViewFrameActions default_actions="none" />
 * </Hints>
 * \endverbatim
 *
 * disables all toolbar buttons while
 *
 * \verbatim
 * <Hints>
 *   <StandardViewFrameActions default_actions="none" />
 *   <ToggleInteractionMode />
 * </Hints>
 * \endverbatim
 *
 * disables all toolbar buttons except for the button that toggles
 * between 2D/3D camera interactions. This type of hint can be used
 * to add only the toolbar buttons that are desired for the view.
 *
 * To show all the default toolbar buttons except for the button that
 * brings up the camera control dialog, use
 *
 * \verbatim
 * <Hints>
 *   <StandardViewFrameActions>
 *     <AdjustCamera visibility="never" />
 *   </StandardViewFrameActions>
 * </Hints>
 * \endverbatim
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqStandardViewFrameActionsImplementation
  : public QObject
  , public pqViewFrameActionsInterface
{
  Q_OBJECT
  Q_INTERFACES(pqViewFrameActionsInterface);

public:
  pqStandardViewFrameActionsImplementation(QObject* parent = nullptr);
  ~pqStandardViewFrameActionsImplementation() override;

  /**
   * This method is called after a frame is assigned to a view. The view may be
   * nullptr, indicating the frame has been assigned to an empty view. Frames are
   * never reused (except a frame assigned to an empty view).
   */
  void frameConnected(pqViewFrame* frame, pqView* view) override;

protected Q_SLOTS:
  /**
   * Called before the "Convert To" menu is shown. We populate the menu with
   * actions for available view types.
   */
  void aboutToShowConvertMenu();

  /**
   * slots for various shortcuts.
   */
  void selectSurfaceCellsTriggered();
  void selectSurfacePointsTriggered();
  void selectFrustumCellsTriggered();
  void selectFrustumPointsTriggered();
  void selectBlocksTriggered();
  void escTriggered();

  /**
   * If a QAction is added to an exclusive QActionGroup, then a checked action
   * cannot be unchecked by clicking on it. We need that to work. Hence, we
   * manually manage the exclusivity of the action group.
   */
  void manageGroupExclusivity(QAction*);

  /**
   * A slot called when any action that can be "cancelled" with Esc is toggled.
   */
  void escapeableActionToggled(bool checked);

  /**
   * A slot called when an interactive selection is toggled
   */
  void interactiveSelectionToggled(bool checked);

  /**
   * This slot is called when a capture view action is triggered.
   */
  void captureViewTriggered();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * called to setup empty frame.
   */
  virtual void setupEmptyFrame(QWidget* frame);

  /**
   * called to add view type independent actions first.
   */
  virtual void addGenericActions(pqViewFrame* frame, pqView* view);

  /**
   * called to add view type independent actions first.
   */
  virtual QActionGroup* addSelectionModifierActions(pqViewFrame* frame, pqView* view);

  /**
   * called to add a separator in the action bar
   */
  virtual void addSeparator(pqViewFrame* frame, pqView* view);

  /**
   * called to add context view actions.
   */
  virtual void addContextViewActions(pqViewFrame* frame, pqContextView* chart_view);

  /**
   * called to add render view actions.
   */
  virtual void addRenderViewActions(pqViewFrame* frame, pqRenderView* view);

  /**
   * called to add actions/decorator for pqSpreadSheetView.
   */
  virtual void addSpreadSheetViewActions(pqViewFrame* frame, pqSpreadSheetView* view);

  /**
   * check the XML hints to see if a button with the given name
   * should be added to the view frame
   */
  virtual bool isButtonVisible(const std::string& buttonName, pqView* view);

  struct ViewType
  {
    QString Label;
    QString Name;
    ViewType() = default;
    ViewType(const QString& label, const QString& name)
      : Label(label)
      , Name(name)
    {
    }
  };

  /**
   * Returns available view types in the application. Used when setting up the
   * "Convert To" menu or when filling up the empty-frame.
   */
  virtual QList<ViewType> availableViewTypes();

  /**
   * Called when user clicks "Convert To" or create a view from the empty
   * frame.
   */
  virtual pqView* handleCreateView(const ViewType& viewType);

  /**
   * This is called either from an action in the "Convert To" menu, or from
   * the buttons on an empty frame.
   */
  void invoked(pqViewFrame*, const ViewType& type, const QString& command);

private:
  Q_DISABLE_COPY(pqStandardViewFrameActionsImplementation)
  QPointer<QShortcut> ShortCutSurfaceCells;
  QPointer<QShortcut> ShortCutSurfacePoints;
  QPointer<QShortcut> ShortCutFrustumCells;
  QPointer<QShortcut> ShortCutFrustumPoints;
  QPointer<QShortcut> ShortCutBlocks;
  QPointer<QShortcut> ShortCutEsc;
  QPointer<QShortcut> ShortCutGrow;
  QPointer<QShortcut> ShortCutShrink;
  static bool ViewTypeComparator(const ViewType& one, const ViewType& two);
};

#endif
