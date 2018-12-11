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
/**
 * @class   vtkKdTreeManager
 * @brief   class used to generate KdTree from unstructured or
 * structured data.
 *
 * ParaView needs to build a KdTree when ordered compositing. The KdTree is
 * either built using the all data in the pipeline when on structure data is
 * present, or using the partitions provided by the structure data's extent
 * translator. This class manages this logic. When structure data's extent
 * translator is to be used, it simply uses vtkKdTreeGenerator. Otherwise, it
 * lets the vtkPKdTree build the optimal partitioning for the data.
*/

#ifndef vtkKdTreeManager_h
#define vtkKdTreeManager_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkSmartPointer.h"                   // needed for vtkSmartPointer.

class vtkPKdTree;
class vtkAlgorithm;
class vtkDataSet;
class vtkDataObject;
class vtkExtentTranslator;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkKdTreeManager : public vtkObject
{
public:
  static vtkKdTreeManager* New();
  vtkTypeMacro(vtkKdTreeManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Add data objects.
   */
  void AddDataObject(vtkDataObject*);
  void RemoveAllDataObjects();
  //@}

  /**
   * Set the optional extent translator to use to get aid in building the
   * KdTree.
   */
  void SetStructuredDataInformation(vtkExtentTranslator* translator, const int whole_extent[6],
    const double origin[3], const double spacing[3]);

  //@{
  /**
   * Get/Set the KdTree managed by this manager.
   */
  void SetKdTree(vtkPKdTree*);
  vtkGetObjectMacro(KdTree, vtkPKdTree);
  //@}

  //@{
  /**
   * Get/Set the number of pieces.
   * Passed to the vtkKdTreeGenerator when SetStructuredDataInformation() is
   * used with non-empty translator.
   */
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  //@}

  /**
   * Rebuilds the KdTree.
   */
  void GenerateKdTree();

protected:
  vtkKdTreeManager();
  ~vtkKdTreeManager() override;

  void AddDataObjectToKdTree(vtkDataObject* data);
  void AddDataSetToKdTree(vtkDataSet* data);

  bool KdTreeInitialized;
  vtkPKdTree* KdTree;
  int NumberOfPieces;

  vtkSmartPointer<vtkExtentTranslator> ExtentTranslator;
  double Origin[3];
  double Spacing[3];
  int WholeExtent[6];

  vtkSetVector3Macro(Origin, double);
  vtkSetVector3Macro(Spacing, double);
  vtkSetVector6Macro(WholeExtent, int);

private:
  vtkKdTreeManager(const vtkKdTreeManager&) = delete;
  void operator=(const vtkKdTreeManager&) = delete;

  class vtkDataObjectSet;
  vtkDataObjectSet* DataObjects;
};

#endif
