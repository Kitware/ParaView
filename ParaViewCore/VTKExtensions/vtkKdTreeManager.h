/*=========================================================================

  Program:   ParaView
  Module:    vtkKdTreeManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKdTreeManager
// .SECTION Description
//

#ifndef __vtkKdTreeManager_h
#define __vtkKdTreeManager_h

#include "vtkObject.h"

class vtkPKdTree;
class vtkAlgorithm;
class vtkDataSet;
class vtkDataObject;

class VTK_EXPORT vtkKdTreeManager : public vtkObject
{
public:
  static vtkKdTreeManager* New();
  vtkTypeMacro(vtkKdTreeManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add producers.
  void AddProducer(vtkAlgorithm*);
  void RemoveProducer(vtkAlgorithm*);
  void RemoveAllProducers();

  // Description:
  // Set the optional producer whose partitioning is used to build the KdTree.
  void SetStructuredProducer(vtkAlgorithm*);

  // Description:
  // Updates all producers are rebuilds the KdTree if the data from any producer
  // changed.
  void Update();

  // Description:
  // Get/Set the KdTree managed by this manager.
  void SetKdTree(vtkPKdTree*);
  vtkGetObjectMacro(KdTree, vtkPKdTree);

  // Description:
  // Get/Set the number of pieces. 
  // Passed to the vtkKdTreeGenerator when StructuredProducer is non-null.
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);

//BTX
protected:
  vtkKdTreeManager();
  ~vtkKdTreeManager();

  void AddDataObjectToKdTree(vtkDataObject *data);
  void AddDataSetToKdTree(vtkDataSet *data);

  bool KdTreeInitialized;
  vtkAlgorithm* StructuredProducer;
  vtkPKdTree* KdTree;
  int NumberOfPieces;
  vtkTimeStamp UpdateTime;
private:
  vtkKdTreeManager(const vtkKdTreeManager&); // Not implemented
  void operator=(const vtkKdTreeManager&); // Not implemented

  class vtkAlgorithmSet;
  vtkAlgorithmSet* Producers;

//ETX
};

#endif

