/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotEdges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPlotEdges
 *
*/

#ifndef vtkPlotEdges_h
#define vtkPlotEdges_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class Node;
class Segment;
class vtkPolyData;
class vtkCollection;
class vtkMultiBlockDataSet;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPlotEdges : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkPlotEdges, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPlotEdges* New();

protected:
  vtkPlotEdges();
  ~vtkPlotEdges() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void Process(vtkPolyData* input, vtkMultiBlockDataSet* output);
  static void ReducePolyData(vtkPolyData* polyData, vtkPolyData* output);

  void ExtractSegments(vtkPolyData* polyData, vtkCollection* segments, vtkCollection* nodes);
  static void ExtractSegmentsFromExtremity(vtkPolyData* polyData, vtkCollection* segments,
    vtkCollection* nodes, char* visitedCells, vtkIdType cellId, vtkIdType pointId, Node* node);
  static void ConnectSegmentsWithNodes(vtkCollection* segments, vtkCollection* nodes);
  static void SaveToMultiBlockDataSet(vtkCollection* segments, vtkMultiBlockDataSet* output);
  static void MergeSegments(vtkCollection* segments, vtkCollection* nodes, Node* node,
    Segment* segmentA, Segment* segmentB);
  static Node* GetNodeAtPoint(vtkCollection* nodes, vtkIdType pointId);
  static void PrintSegments(vtkCollection* segments);

private:
  vtkPlotEdges(const vtkPlotEdges&) = delete;
  void operator=(const vtkPlotEdges&) = delete;
};

#endif
