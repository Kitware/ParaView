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

#include "vtkPVInformation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

#include <vector> // needed for internal API

class VTKREMOTINGVIEWS_EXPORT vtkPVStreamingPiecesInformation : public vtkPVInformation
{
public:
  static vtkPVStreamingPiecesInformation* New();
  vtkTypeMacro(vtkPVStreamingPiecesInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object. Calls AddInformation(info, 0).
   */
  void AddInformation(vtkPVInformation* info) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  /**
   * API to access the internal keys.
   */
  void GetKeys(std::vector<unsigned int>& keys) const;

protected:
  vtkPVStreamingPiecesInformation();
  ~vtkPVStreamingPiecesInformation() override;

private:
  vtkPVStreamingPiecesInformation(const vtkPVStreamingPiecesInformation&) = delete;
  void operator=(const vtkPVStreamingPiecesInformation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
