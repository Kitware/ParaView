/*=========================================================================

Copyright (c) 1998-2005 Kitware Inc. 28 Corporate Drive, Suite 204,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced,
distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO
PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkCPCellFieldBuilder - Class for specifying cell fields over grids.
// .SECTION Description
// Class for specifying cell data fields over grids for a test driver.  

#ifndef __vtkCPCellFieldBuilder_h
#define __vtkCPCellFieldBuilder_h

#include "vtkCPFieldBuilder.h"

class VTK_EXPORT vtkCPCellFieldBuilder : public vtkCPFieldBuilder
{
public:
  static vtkCPCellFieldBuilder* New();
  vtkTypeMacro(vtkCPCellFieldBuilder, vtkCPFieldBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a field on Grid. 
  virtual void BuildField(unsigned long TimeStep, double Time,
                          vtkDataSet* Grid);

  // Description:
  // Return the highest order of discretization of the field.
  //virtual unsigned int GetHighestFieldOrder();

protected:
  vtkCPCellFieldBuilder();
  ~vtkCPCellFieldBuilder();

private:
  vtkCPCellFieldBuilder(const vtkCPCellFieldBuilder&); // Not implemented
  void operator=(const vtkCPCellFieldBuilder&); // Not implemented
};

#endif
