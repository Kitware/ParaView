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
 *
 * @deprecated in ParaView 5.12.0. See `vtkCLIOptions` instead.
 * See https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4951 for
 * developer guidelines.
 */

#ifndef vtkPVOptionsXMLParser_h
#define vtkPVOptionsXMLParser_h

#include "vtkCommandOptionsXMLParser.h"
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_12_0
#include "vtkRemotingCoreModule.h"  //needed for exports

class vtkCommandOptions;

class VTKREMOTINGCORE_EXPORT vtkPVOptionsXMLParser : public vtkCommandOptionsXMLParser
{
public:
  PARAVIEW_DEPRECATED_IN_5_12_0("Use `vtkCLIOptions` instead")
  static vtkPVOptionsXMLParser* New();
  vtkTypeMacro(vtkPVOptionsXMLParser, vtkCommandOptionsXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVOptionsXMLParser() = default;
  ~vtkPVOptionsXMLParser() override = default;

  void SetProcessType(const char* ptype) override;

private:
  vtkPVOptionsXMLParser(const vtkPVOptionsXMLParser&) = delete;
  void operator=(const vtkPVOptionsXMLParser&) = delete;
};

#endif // #ifndef vtkPVOptionsXMLParser_h
