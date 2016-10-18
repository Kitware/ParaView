/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVStreamingPiecesInformation
 * @brief   information object used by
 * vtkSMDataDeliveryManager to get information about representations that have
 * pieces to stream from the data-server.
 *
 * vtkPVStreamingPiecesInformation is an information object used by
 * vtkSMDataDeliveryManager to get information about representations that have
 * pieces to stream from the data-server.
*/

#ifndef vtkPVStreamingPiecesInformation_h
#define vtkPVStreamingPiecesInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" // needed for export macro
#include "vtkPVInformation.h"

#include <vector> // needed for internal API

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVStreamingPiecesInformation
  : public vtkPVInformation
{
public:
  static vtkPVStreamingPiecesInformation* New();
  vtkTypeMacro(vtkPVStreamingPiecesInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*);

  /**
   * Merge another information object. Calls AddInformation(info, 0).
   */
  virtual void AddInformation(vtkPVInformation* info);

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //@}

  /**
   * API to access the internal keys.
   */
  void GetKeys(std::vector<unsigned int>& keys) const;

protected:
  vtkPVStreamingPiecesInformation();
  ~vtkPVStreamingPiecesInformation();

private:
  vtkPVStreamingPiecesInformation(const vtkPVStreamingPiecesInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVStreamingPiecesInformation&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
