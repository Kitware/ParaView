// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqScalarsToColors_h
#define pqScalarsToColors_h

#include "pqProxy.h"
#include <QList>
#include <QPair>
#include <QVariant>

class pqRenderViewBase;
class pqScalarBarRepresentation;
class pqScalarsToColorsInternal;

/**
 * pqScalarsToColors is a represents a vtkScalarsToColors proxy.
 */
class PQCORE_EXPORT pqScalarsToColors : public pqProxy
{
  Q_OBJECT
public:
  pqScalarsToColors(const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server,
    QObject* parent = nullptr);
  ~pqScalarsToColors() override;

  /**
   * Returns the first scalar bar visible in the given render module, if any.
   */
  pqScalarBarRepresentation* getScalarBar(pqRenderViewBase* ren) const;

  /**
   * Returns if the lookup table's scalar range is locked.
   */
  bool getScalarRangeLock() const;

  /**
   * Sets the scalar range.
   * Does not consider the ScalarRangeLock. Moves all control points uniformly
   * to fit the new range.
   */
  void setScalarRange(double min, double max);

  /**
   * Returns the current scalar range. If number of RGBPoints is 0, then the
   * scalar range is not defined (in which case this method returns (0, 0).
   */
  QPair<double, double> getScalarRange() const;

  /**
   * Returns true if a log scale is being used.
   */
  bool getUseLogScale() const;

  enum Mode
  {
    MAGNITUDE = 0,
    COMPONENT = 1
  };

  // Set the color mode (component/magnitude) and
  // component to color by. When mode is magnitude, component is ignored.
  void setVectorMode(Mode mode, int component);

  // Returns the vector mode and component used by the Lookup table.
  // When vector mode is MAGNITUDE, value returned by
  // getVectorComponent() is indeterminate.
  Mode getVectorMode() const;
  int getVectorComponent() const;

  /**
   * Convenience method to update the titles for all color legends showing this
   * lookup table. This only updates the component part of the title.
   */
  void updateScalarBarTitles(const QString& component);

  enum RangeScalingModes
  {
    GROW_ON_MODIFIED, /* only when a pipeline object is explicitly modified */
    GROW_ON_UPDATED   /* any time the pipeline updates */
  };

  /**
   * This merely update the application settings with correct values for the
   * temporal range scaling mode. Components that scale scalar range should
   * look at this setting to determine how the scaling should be done.
   */

  /**
   * This provides access to the application settings that controls when color
   * table ranges are updated by the application. Components that scale scalar
   * range should look at this setting to determine how the scaling should be
   * done.
   */
  static void setColorRangeScalingMode(int);
  static int colorRangeScalingMode(int default_value = GROW_ON_MODIFIED);

Q_SIGNALS:
  /**
   * signal fired when the "VectorMode" or "VectorComponent" properties are
   * modified.
   */
  void componentOrModeChanged();

public Q_SLOTS:
  // This method checks if this LUT is used by any display,
  // if not, it hides all the scalars bar showing this LUT.
  void hideUnusedScalarBars();

  /**
   * Set the scalar range lock.
   */
  void setScalarRangeLock(bool lock);

  /**
   * Triggers a build on the lookup table.
   */
  void build();

protected Q_SLOTS:
  /**
   * Checks to make sure that the range is compatible with the log flag and
   * adjusts it if necessary.
   */
  void checkRange();

private:
  pqScalarsToColorsInternal* Internal;
};

#endif
