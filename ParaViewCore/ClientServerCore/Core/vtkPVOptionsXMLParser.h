/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOptionsXMLParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVOptionsXMLParser
 * @brief   ParaView options storage
 *
 * An object of this class represents a storage for ParaView options
 *
 * These options can be retrieved during run-time, set using configuration file
 * or using Command Line Arguments.
*/

#ifndef vtkPVOptionsXMLParser_h
#define vtkPVOptionsXMLParser_h

#include "vtkCommandOptionsXMLParser.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
class vtkCommandOptions;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVOptionsXMLParser : public vtkCommandOptionsXMLParser
{
public:
  static vtkPVOptionsXMLParser* New();
  vtkTypeMacro(vtkPVOptionsXMLParser, vtkCommandOptionsXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVOptionsXMLParser() {}
  ~vtkPVOptionsXMLParser() override {}

  void SetProcessType(const char* ptype) override;

private:
  vtkPVOptionsXMLParser(const vtkPVOptionsXMLParser&) = delete;
  void operator=(const vtkPVOptionsXMLParser&) = delete;
};

#endif // #ifndef vtkPVOptionsXMLParser_h
