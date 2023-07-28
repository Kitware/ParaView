// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSearchBox_h
#define pqSearchBox_h

#include "pqComponentsModule.h"
#include <QScopedPointer>
#include <QWidget>

class QSettings;
/**
 * pqSearchBox is a line edit and an advanced tool button in the same
 * layout. Most of the time, the text of the line edit is used to
 * filter the properties of the panel.
 * The pqSearchBox adds some functionnalities:
 *   - When the user presses Esc, the current search text is cleared.
 *   - The advanced button has different configuration that allow to choose
 *   its behavior. See the advancedSearchEnabled and the settingKey properties
 *   for more details.
 * The search box text can be accessed through the text property. The same
 * goes for the advanced button state through the advancedSearchActive
 * property (although it depends on the current advancedSearchEnabled).
 */
class PQCOMPONENTS_EXPORT pqSearchBox : public QWidget
{
  Q_OBJECT

  /**
   * The text property allow to access the text in the search box line edit.
   * Default is empty.
   * \sa QLineEdit
   */
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged USER true)

  /**
   * The placeholderText property mirrors the line edit placeholderText
   * property.
   * Default text is "Search... (use Esc to clear text)"
   * \sa QLineEdit
   */
  Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)

  /**
   * The advancedSearchActive allows the user to access/control the advanced
   * search button state depending on the current advancedSearchEnabled
   * property.
   * \sa advancedSearchEnabled property
   */
  Q_PROPERTY(bool advancedSearchActive READ isAdvancedSearchActive WRITE setAdvancedSearchActive
      NOTIFY advancedSearchActivated)

  /**
   * This property governs whether the advanced search button is enabled
   * and visible (enabled == visible).
   * By default the advanced search button is disabled/hidden.
   */
  Q_PROPERTY(bool advancedSearchEnabled READ isAdvancedSearchEnabled WRITE setAdvancedSearchEnabled)

  /**
   * When the advanced search is enabled, the button can save/restore its
   * state from the settings using the settingKey property. If the key
   * is empty then the settings have no influence on the advanced button
   * state.
   * In the following cases, the widget will restore the advanced button
   * state to the setting key value:
   *  - When changing the setting key to a new valid (i.e. non-empty) key
   *  - When enabling the advanced button with a valid key already present.
   * By default the setting key is empty, meaning that the advanced button
   * state isn't saved in the settings;
   */
  Q_PROPERTY(QString settingKey READ settingKey WRITE setSettingKey NOTIFY settingKeyChanged)

public:
  typedef QWidget Superclass;

  pqSearchBox(QWidget* parent = nullptr);
  pqSearchBox(
    bool advancedSearchEnabled, const QString& settingKey = "", QWidget* parent = nullptr);

  ~pqSearchBox() override;

  /**
   * Returns whether the advanced button is activated.
   * advancedSearchActive property
   */
  bool isAdvancedSearchActive() const;

  /**
   * Get the current search text.
   * \sa text property, QLineEdit
   */
  QString text() const;

  /**
   * Set/Get the current search text.
   * \sa placeholderText property, QLineEdit
   */
  QString placeholderText() const;
  void setPlaceholderText(const QString& text);

  /**
   * Get the current setting key used to save the advanced button check state.
   * \sa settingKey property
   */
  QString settingKey() const;

  /**
   * Convenience method to access the settings. It should not be used to
   * modify the advanced button state. Use setAdvancedSearchActive() instead.
   */
  QSettings* settings() const;

  /**
   * Return whether the advanced search button is visible/enabled.
   * \sa advancedSearchEnabled property
   */
  bool isAdvancedSearchEnabled() const;

Q_SIGNALS:
  /**
   * Sent when the advanced button is toggled. Note that no signal is
   * sent when the configuration is None.
   * \sa advancedSearchActive property
   */
  void advancedSearchActivated(bool);

  /**
   * Sent whenever the search text is changed.
   * \sa text property, QLineEdit
   */
  void textChanged(const QString&);

  /**
   * Sent whenever the setting key was changed.
   * \sa settingKey property
   */
  void settingKeyChanged(const QString&);

public Q_SLOTS:
  /**
   * Toggle the advanced search button. This is a no-op when the configuration
   * is None.
   * \sa advancedSearchActive property
   */
  void setAdvancedSearchActive(bool use);

  /**
   * Set the search text.
   * \sa text property, QLineEdit
   */
  void setText(const QString& text);

  /**
   * Set the new setting key that will be used to restore/save the advanced
   * button check state. If the given key is valid (i.e. not empty), the
   * button state will be restored from the key value.
   * The old key is left unchanged in the setting to whatever its last value
   * was. Although returned, removing (or not) the old key is up to the user.
   * \sa settingKey property
   */
  QString setSettingKey(const QString& key);

  /**
   * Set whether the advanced button is visible/enabled.
   * \sa advancedSearchEnabled property
   */
  void setAdvancedSearchEnabled(bool enable);

protected:
  void keyPressEvent(QKeyEvent* e) override;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void onAdvancedButtonClicked(bool);
  void updateSettings();
  void updateFromSettings();

private:
  Q_DISABLE_COPY(pqSearchBox)

  class pqInternals;
  friend class pqInternals;

  const QScopedPointer<pqInternals> Internals;
  QString SettingKey;
};

#endif
