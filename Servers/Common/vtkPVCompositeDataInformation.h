/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeDataInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeDataInformation - Light object for holding composite data information.
// .SECTION Description
// vtkPVCompositeDataInformation is used to copy the meta information of 
// a composite dataset from server to client. It holds a vtkPVDataInformation
// for each block of the composite dataset.
// .SECTION See Also
// vtkHierarchicalDataSet vtkPVDataInformation

#ifndef __vtkPVCompositeDataInformation_h
#define __vtkPVCompositeDataInformation_h

#include "vtkPVInformation.h"

class vtkPVDataInformation;
//BTX
struct vtkPVCompositeDataInformationInternals;
//ETX

class VTK_EXPORT vtkPVCompositeDataInformation : public vtkPVInformation
{
public:
  static vtkPVCompositeDataInformation* New();
  vtkTypeRevisionMacro(vtkPVCompositeDataInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // True if data passed in CopyFromStream() is vtkHierarchicalDataSet
  // or sub-class, false otherwise. Is only valid after CopyFromObject()
  // has been called.
  vtkGetMacro(DataIsComposite, int);

  // Description:
  // Clears all internal data structures.
  virtual void Initialize();

  // Description:
  // Returns the number of levels.
  unsigned int GetNumberOfGroups();

  // Description:
  // Given a level, returns the number of datasets.
  unsigned int GetNumberOfDataSets(unsigned int level);

  // Description:
  // Given a level and index, returns the data information.
  vtkPVDataInformation* GetDataInformation(unsigned int level,
                                           unsigned int idx);

protected:
  vtkPVCompositeDataInformation();
  ~vtkPVCompositeDataInformation();

  int DataIsComposite;

private:
  vtkPVCompositeDataInformationInternals* Internal;

  vtkPVCompositeDataInformation(const vtkPVCompositeDataInformation&); // Not implemented
  void operator=(const vtkPVCompositeDataInformation&); // Not implemented
};

#endif
