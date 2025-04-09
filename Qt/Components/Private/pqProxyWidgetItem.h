// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include <QObject>

#include <QPointer> // for QPointer members

class pqPropertyWidget;
class pqProxyWidget;
class vtkSMProxy;

class QFrame;
class QGridLayout;

/**
 * @brief pqProxyWidgetItem represents a single widget for a property/properties.
 *
 * @details While pqProxyWidgetItem mainly handles a pqPropertyWidget with its label,
 * it is also used for propertyGroups.
 *
 * It can be added to a QGridLayout (see appendToLayout()), so underlying QWidgets
 * are consistently added.
 */
class pqProxyWidgetItem : public QObject
{
  typedef QObject Superclass;

public:
  ~pqProxyWidgetItem() override;

  /**
   * Create a simple pqProxyWidgetItem
   */
  static pqProxyWidgetItem* newItem(pqPropertyWidget* widget, const QString& label, bool advanced,
    bool informationOnly, const QStringList& searchTags, pqProxyWidget* parentObj);

  /**
   * Creates a new item for a property group. Use this overload when creating
   * an item for a group where there's a single widget for all the properties
   * in that group.
   */
  static pqProxyWidgetItem* newGroupItem(pqPropertyWidget* widget, const QString& label,
    bool showSeparators, bool advanced, const QStringList& searchTags, pqProxyWidget* parentObj);

  /**
   * Creates a new item for a property group with several widgets (for
   * individual properties in the group).
   */
  static pqProxyWidgetItem* newMultiItemGroupItem(const QString& group_label,
    pqPropertyWidget* widget, const QString& widget_label, bool showSeparators, bool advanced,
    bool informationOnly, const QStringList& searchTags, pqProxyWidget* parentObj);

  /**
   * Create a simple horizontal line. Useful to add a visible but light separation.
   */
  static QFrame* newHLine(QWidget* parent);

  /**
   * Create a separator for a new Group widget.
   * This is mainly more spacing and an horizontal line.
   */
  static QWidget* newGroupSeparator(QWidget* parent);

  /**
   * Return the underlying pqPropertyWidget.
   */
  pqPropertyWidget* propertyWidget() const;

  /**
   * Add representation to the list of representations
   * that forces visibility to be "default", bypassing the "Advanced" option.
   */
  void appendToDefaultVisibilityForRepresentations(const QString& repr);

  /**
   * Utility methods forwarded to the Property widget.
   */
  ///@{
  void apply() const;
  void reset() const;
  void select() const;
  void deselect() const;
  ///@}

  /**
   * Return true if the widget should be shown in the current configuration:
   * advanced mode, filtering text and optional decorators.
   */
  bool canShowWidget(bool show_advanced, const QString& filterText, vtkSMProxy* proxy) const;

  /**
   * Return true if the widget should be enabled.
   * Return false if the property is Information only or if at least one decorator say so.
   */
  bool enableWidget() const;

  /**
   * Return true if the property is advanced.
   */
  bool isAdvanced(vtkSMProxy* proxy) const;

  /**
   * Show the underlying widgets
   * Separators are shown only when needed (i.e. if another item is around).
   */
  void show(const pqProxyWidgetItem* prevVisibleItem, bool enabled = true,
    bool show_advanced = false) const;

  /**
   * Hide the underlying widgets.
   */
  void hide() const;

  /**
   * Adds widgets to the layout. This is a little greedy. It adds everything
   * that could be potentially shown to the layout. We control visibilities
   * of things like headers and footers dynamically in show()/hide().
   */
  void appendToLayout(QGridLayout* glayout, bool singleColumn);

private:
  Q_DISABLE_COPY(pqProxyWidgetItem);

  // Regular expression with tags used to match search text.
  QStringList SearchTags;
  bool Advanced = false;
  bool InformationOnly = false;

  QPointer<QWidget> GroupHeader;
  QPointer<QWidget> GroupFooter;
  QPointer<QWidget> LabelWidget;
  QPointer<pqPropertyWidget> PropertyWidget;
  QStringList DefaultVisibilityForRepresentations;
  bool Group = false;
  QString GroupTag;

  pqProxyWidgetItem(
    QObject* parentObj, bool advanced, bool informationOnly, const QStringList& searchTags);
};
