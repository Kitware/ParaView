/*=========================================================================

   Program: ParaView
   Module:    pqScalarsToColors.h

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
    QObject* parent = NULL);
  ~pqScalarsToColors() override;

  /**
  * Returns the first scalar bar visible in the given render module,
  * if any.
  */
  pqScalarBarRepresentation* getScalarBar(pqRenderViewBase* ren) const;

  /**
  * Returns if the lookup table's scalar range is locked.
  */
  bool getScalarRangeLock() const;

  /**
  * Sets the scalar range.
  * Does not consider the ScalarRangeLock. Moves all control points
  * uniformly to fit the new range.
  */
  void setScalarRange(double min, double max);

  /**
  * Returns the current scalar range. If number of RGBPoints is 0,
  * then the scalar range is not defined (in which case this method
  * returns (0, 0).
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
  * Checks to make sure that the range is compatible with the log flag
  * and adjusts it if necessary.
  */
  void checkRange();

private:
  pqScalarsToColorsInternal* Internal;
};

#endif
