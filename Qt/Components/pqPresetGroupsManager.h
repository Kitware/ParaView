/*=========================================================================

   Program: ParaView
   Module:  pqPresetGroupManager.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/

#ifndef pqPresetGroupManager_h
#define pqPresetGroupManager_h

#include "pqComponentsModule.h" // for exports

#include <QList>
#include <QMap>
#include <QObject>

/**
 * pqPresetGroupsManager is an object to manage the groups of color map
 * presets displayed in the pqPresetDialog.  A pqPresetGroupsManager is
 * created by the pqPVApplicationCore and registered as a manager.  To get
 * it:
 *
 * @code{cpp}
 * auto groupManager = qobject_cast<pqPresetGroupsManager*>(
 *     pqApplicationCore::instance()->manager("PRESET_GROUP_MANAGER"));
 * @endcode
 *
 * The group manager loads the groups of presets from a json file with this format:
 * @code{json}
 * [
 *   {
 *     "groupName": "default",
 *     "presets": [
 *       "Cool to Warm",
 *       "Cool to Warm (Extended)",
 *       "Black-Body Radiation",
 *       "X Ray",
 *       "Inferno (matplotlib)",
 *       "Black, Blue and White",
 *       "Blue Orange (divergent)",
 *       "Viridis (matplotlib)",
 *       "Gray and Red",
 *       "Linear Green (Gr4L)",
 *       "Cold and Hot",
 *       "Blue - Green - Orange",
 *       "Rainbow Desaturated",
 *       "Yellow - Gray - Blue",
 *       "Rainbow Uniform",
 *       "jet"
 *     ]
 *   },
 *   {
 *     "groupName": "myTestGroup",
 *     "presets": [
 *       "Reds",
 *       "Blues",
 *       "Greens",
 *       "Oranges",
 *       "Br0rYl",
 *       "erdc_blue2yellow"
 *     ]
 *   }
 * ]
 * @endcode
 */
class PQCOMPONENTS_EXPORT pqPresetGroupsManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * Create a new groups manager.  Custom applications should use the one created by
   * pqPVApplicationCore (see class description).
   */
  pqPresetGroupsManager(QObject* p);
  ~pqPresetGroupsManager();

  /**
   * Returns the number of groups the manager knows of.
   */
  int numberOfGroups();
  /**
   * Returns the number of presets in the given group.
   */
  int numberOfPresetsInGroup(const QString& groupName);
  /**
   * Returns the rank (index) of the given preset in the given group.
   * Returns -1 if the preset is not in the group (the preset dialog relies on this).
   */
  int presetRankInGroup(const QString& presetName, const QString& groupName);
  /**
   * Returns a list of the group names the manager knows of.
   */
  QList<QString> groupNames();
  /**
   * Returns the name of the ith group in the manager's list of groups.
   */
  QString groupName(int i);

  /**
   * Load groups from the given JSON string (for the format see the class
   * documentation) and add the groups to the groups in the manager.  This
   * is an append operation so the groups already in the manager will not be removed.
   * If the json string has a group with the same name as one already in the
   * manager, then the color maps in that group will be appended to the existing group.
   */
  void loadGroups(const QString& json);

  /**
   * Clears all groups in the manager and then loads groups from the given JSON string
   * (see loadGroups).  This is an override operation instead of the append provided by
   * loadGroups.
   */
  void replaceGroups(const QString& json);

Q_SIGNALS:
  /**
   * Fired whenever loadGroups or replaceGroups is called and new group information
   * is available.
   */
  void groupsUpdated();

private:
  QList<QString> GroupNames;
  QMap<QString, QList<QString> > Groups;

  /**
   * Clears all groups the manager knows.  This resets the internal state so that
   * ParaView's default groups can be overridden.  This is private since pqPresetDialog
   * does not handle the case where there are no groups.  Use replaceGroups to clear
   * the groups and replace them with a new set of groups from a JSON string.
   */
  void clearGroups();
};

#endif
