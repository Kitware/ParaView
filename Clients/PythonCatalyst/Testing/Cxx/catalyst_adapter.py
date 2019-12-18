global_catalyst_coprocessor = None

def initialize(coprocessing_script):
    global global_catalyst_coprocessor

    import paraview
    paraview.options.batch = True
    paraview.options.symmetric = True

    from paraview.vtk.vtkPVClientServerCoreCore import vtkProcessModule

def coprocess(dataset, timestep, time):
    pass

def finalize():
    pass
