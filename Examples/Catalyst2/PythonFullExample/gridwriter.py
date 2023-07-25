# script-version: 2.0
# Sample Catalyst Python script to generate data outputs.
# One can use this directly or create a modified version
# to customize write frequency, output directory etc.

#---------------------------------------------------------
# Input parameters
#---------------------------------------------------------
# Specify the Catalyst channel name
catalystChannel = "grid"

# Specify the write frequency
frequency = 5

# Specify the output directory. Ideally, this should be an
# absolute path to avoid confusion.
outputDirectory = "./datasets"

# Specify extractor type to use, if any.
# e.g. for CSV, set extractorType to 'CSV'
extractorType = None

#---------------------------------------------------------
from paraview.simple import *

# Specify the Catalyst channel name
def create_extractor(data):
    if extractorType is not None:
        return CreateExtractor(extractorType, data, registrationName=extractorType)

    grid = data.GetClientSideObject().GetOutputDataObject(0)
    if grid.IsA('vtkImageData'):
        return CreateExtractor('VTI', data, registrationName='VTI')
    elif grid.IsA('vtkRectilinearGrid'):
        return CreateExtractor('VTR', data, registrationName='VTR')
    elif grid.IsA('vtkStructuredGrid'):
        return CreateExtractor('VTS', data, registrationName='VTS')
    elif grid.IsA('vtkPolyData'):
        return CreateExtractor('VTP', data, registrationName='VTP')
    elif grid.IsA('vtkUnstructuredGrid'):
        return CreateExtractor('VTU', data, registrationName='VTU')
    elif grid.IsA('vtkUniformGridAMR'):
        return CreateExtractor('VTH', data, registrationName='VTH')
    elif grid.IsA('vtkMultiBlockDataSet'):
        return CreateExtractor('VTM', data, registrationName='VTM')
    elif grid.IsA('vtkPartitionedDataSet'):
        return CreateExtractor('VTPD', data, registrationName='VTPD')
    elif grid.IsA('vtkPartitionedDataSetCollection'):
        return CreateExtractor('VTPC', data, registrationName='VTPC')
    elif  grid.IsA('vtkHyperTreeGrid'):
        return CreateExtractor('HTG', data, registrationName='HTG')
    else:
        raise RuntimeError("Unsupported data type: %s. Check that the adaptor "
                           "is providing channel named %s",
                           grid.GetClassName(), catalystChannel)


# Pipeline
data = TrivialProducer(registrationName=catalystChannel)

# Returns extractor type based on data (or you can manually specify
extractor = create_extractor(data)

# ------------------------------------------------------------------------------
# Catalyst options
from paraview import catalyst
options = catalyst.Options()
options.ExtractsOutputDirectory = outputDirectory
options.GlobalTrigger.Frequency = frequency

# ------------------------------------------------------------------------------
if __name__ == '__main__':
    from paraview.simple import SaveExtractsUsingCatalystOptions
    # Code for non in-situ environments; if executing in post-processing
    # i.e. non-Catalyst mode, let's generate extracts using Catalyst options
    SaveExtractsUsingCatalystOptions(options)
