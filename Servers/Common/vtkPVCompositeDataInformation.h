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
// vtkHierarchicalBoxDataSet vtkPVDataInformation

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
  // Clears all internal data structures.
  virtual void Initialize();

  // Description:
  // Returns the number of children.
  unsigned int GetNumberOfChildren();

  // Description:
  // Returns the information for the data object at the given index. If the
  // child is a composite dataset itself, then the return vtkPVDataInformation
  // will have the CompositeDataInformation set appropriately.
  vtkPVDataInformation* GetDataInformation(unsigned int idx);

  // Description:
  // Get/Set if the data is multipiece. If so, then GetDataInformation() will
  // always return NULL. For vtkMultiblockDataSet, we don't collect information
  // about individual pieces. One can however, query the number of pieces by
  // using GetNumberOfChildren().
  vtkGetMacro(DataIsMultiPiece, int);

  // Description:
  // Returns if the dataset is a composite dataset.
  vtkGetMacro(DataIsComposite, int);

  // TODO:
  // Add API to obtain meta data information for each of the children. 

  // Description:
  // Get the flat-index range for the composite dataset.
  vtkGetMacro(FlatIndexMax, unsigned int);

//BTX
protected:
  vtkPVCompositeDataInformation();
  ~vtkPVCompositeDataInformation();

  int DataIsMultiPiece;
  int DataIsComposite;
  unsigned int FlatIndexMax;
  
  unsigned int NumberOfPieces;
  vtkSetMacro(NumberOfPieces, unsigned int);

  friend class vtkPVDataInformation;
  vtkPVDataInformation* GetDataInformationForCompositeIndex(int *index);
  
private:
  vtkPVCompositeDataInformationInternals* Internal;

  vtkPVCompositeDataInformation(const vtkPVCompositeDataInformation&); // Not implemented
  void operator=(const vtkPVCompositeDataInformation&); // Not implemented
//ETX
};

#endif
