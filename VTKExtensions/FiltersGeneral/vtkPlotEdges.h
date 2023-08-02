// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPlotEdges
 *
 */

#ifndef vtkPlotEdges_h
#define vtkPlotEdges_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkCollection;
class vtkPartitionedDataSet;
class vtkPolyData;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPlotEdges : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkPlotEdges, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPlotEdges* New();

protected:
  vtkPlotEdges();
  ~vtkPlotEdges() override;

  class Node;
  class Segment;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ProcessMultiBlockDataSet(vtkMultiBlockDataSet* input, vtkMultiBlockDataSet* output);
  void ProcessPartitionedDataSet(vtkPartitionedDataSet* input, vtkMultiBlockDataSet* output);
  void Process(vtkPolyData* input, vtkMultiBlockDataSet* output);
  static void ReducePolyData(vtkPolyData* polyData, vtkPolyData* output);

  void ExtractSegments(vtkPolyData* polyData, vtkCollection* segments, vtkCollection* nodes);
  static void ExtractSegmentsFromExtremity(vtkPolyData* polyData, vtkCollection* segments,
    vtkCollection* nodes, char* visitedCells, vtkIdType cellId, vtkIdType pointId, Node* node);
  static void ConnectSegmentsWithNodes(vtkCollection* segments, vtkCollection* nodes);
  static void SaveToMultiBlockDataSet(vtkCollection* segments, vtkMultiBlockDataSet* output);
  static void MergeSegments(vtkCollection* segments, vtkCollection* nodes, Node* node,
    Segment* segmentA, Segment* segmentB);
  static vtkPlotEdges::Node* GetNodeAtPoint(vtkCollection* nodes, vtkIdType pointId);
  static void PrintSegments(vtkCollection* segments);

private:
  vtkPlotEdges(const vtkPlotEdges&) = delete;
  void operator=(const vtkPlotEdges&) = delete;
};

#endif
