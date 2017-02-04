/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartWarning.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
#include "vtkPVClientServerCoreRenderingModule.h" // For export macro

class vtkChart;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkChartWarning : public vtkBlockItem
{
public:
  static vtkChartWarning* New();
  vtkTypeMacro(vtkChartWarning, vtkBlockItem);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Paint event for the item.
   */
  virtual bool Paint(vtkContext2D* painter) VTK_OVERRIDE;

  /**
   * Returns true if the supplied x, y coordinate is inside the item.
   */
  virtual bool Hit(const vtkContextMouseEvent& mouse) VTK_OVERRIDE;

  vtkSetMacro(TextPad, double);
  vtkGetMacro(TextPad, double);

protected:
  vtkChartWarning();
  ~vtkChartWarning();

  bool ArePlotsImproperlyScaled(vtkChart*);

  double TextPad;

private:
  vtkChartWarning(const vtkChartWarning&) VTK_DELETE_FUNCTION;
  void operator=(const vtkChartWarning&) VTK_DELETE_FUNCTION;
};

#endif // vtkChartWarning_h
