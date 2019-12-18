/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVSelectionInformation
 * @brief   Used to gather selection information
 *
 * Used to get information about selection from server to client.
 * The results are stored in a vtkSelection.
 * @sa
 * vtkSelection
*/

#ifndef vtkPVSelectionInformation_h
#define vtkPVSelectionInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkClientServerStream;
class vtkPVXMLElement;
class vtkSelection;

class VTKREMOTINGVIEWS_EXPORT vtkPVSelectionInformation : public vtkPVInformation
{
public:
  static vtkPVSelectionInformation* New();
  vtkTypeMacro(vtkPVSelectionInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Copy information from a selection to internal datastructure.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  //@{
  /**
   * Returns the selection. Selection is created and populated
   * at the end of GatherInformation.
   */
  vtkGetObjectMacro(Selection, vtkSelection);
  //@}

protected:
  vtkPVSelectionInformation();
  ~vtkPVSelectionInformation() override;

  void Initialize();
  vtkSelection* Selection;

private:
  vtkPVSelectionInformation(const vtkPVSelectionInformation&) = delete;
  void operator=(const vtkPVSelectionInformation&) = delete;
};

#endif
