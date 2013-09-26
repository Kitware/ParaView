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

// .NAME vtkChartWarning - a vtkContextItem that draws a block (optional label).
//
// .SECTION Description
// This is a vtkContextItem that can be placed into a vtkContextScene. It draws
// a block of the given dimensions, and reacts to mouse events.

#ifndef __vtkChartWarning_h
#define __vtkChartWarning_h

#include "vtkPVClientServerCoreRenderingModule.h" // For export macro
#include "vtkBlockItem.h"

class vtkChart;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkChartWarning : public vtkBlockItem
{
public:
  static vtkChartWarning* New();
  vtkTypeMacro(vtkChartWarning,vtkBlockItem);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Paint event for the item.
  virtual bool Paint(vtkContext2D* painter);

//BTX
  // Description:
  // Returns true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent& mouse);
//ETX

  vtkSetMacro(TextPad,double);
  vtkGetMacro(TextPad,double);

//BTX
protected:
  vtkChartWarning();
  ~vtkChartWarning();

  bool ArePlotsImproperlyScaled(vtkChart*);

  double TextPad;

private:
  vtkChartWarning(const vtkChartWarning&); // Not implemented.
  void operator = (const vtkChartWarning&); // Not implemented.
//ETX
};

#endif //__vtkChartWarning_h
