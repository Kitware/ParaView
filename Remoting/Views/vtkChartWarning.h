// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkChartWarning
 * @brief   a vtkContextItem that draws a block (optional label).
 *
 *
 * This is a vtkContextItem that can be placed into a vtkContextScene. It draws
 * a block of the given dimensions, and reacts to mouse events.
 */

#ifndef vtkChartWarning_h
#define vtkChartWarning_h

#include "vtkBlockItem.h"
#include "vtkRemotingViewsModule.h" // For export macro

class vtkChart;

class VTKREMOTINGVIEWS_EXPORT vtkChartWarning : public vtkBlockItem
{
public:
  static vtkChartWarning* New();
  vtkTypeMacro(vtkChartWarning, vtkBlockItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Paint event for the item.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Returns true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  vtkSetMacro(TextPad, double);
  vtkGetMacro(TextPad, double);

protected:
  vtkChartWarning();
  ~vtkChartWarning() override;

  bool ArePlotsImproperlyScaled(vtkChart*);

  double TextPad;

private:
  vtkChartWarning(const vtkChartWarning&) = delete;
  void operator=(const vtkChartWarning&) = delete;
};

#endif // vtkChartWarning_h
