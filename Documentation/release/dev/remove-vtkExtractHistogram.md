# Remove vtkExtractHistogram from paraview project

'vtkExtractHistogram' has been moved upstream into the VTK project. Hence it
has been removed from the paraview project. The 'vtkPExtractHistogram' subclass
has been modified to reflect the normalize/accumulate features added to
'vtkExtractHistogram'.
