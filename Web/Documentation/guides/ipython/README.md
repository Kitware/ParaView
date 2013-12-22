# ParaViewWeb and iPython/notebook

## Introduction

The goal of that integration is to be able from iPython to create
a PostProcessing notebook that will let the user create post-processing
pipeline and visualization inside iPython. This will rely underneath on
ParaViewWeb and therefore ParaView on the computational cluster. All the
interactions will be achieved within iPython.

## iPython-notebook

### Installation

This documentation will create a virtual environment where we will install
our iPython/notebook configuration.

In order to create a Virtual Environment for Python you will need to have
virtualenv which can be downloaded here:

- https://pypi.python.org/pypi/virtualenv

Virtualenv don't need to be installed and can simply be run from the command
line in order to create a new virtual environment.

    $ wget https://pypi.python.org/packages/source/v/virtualenv/virtualenv-1.10.1.tar.gz
    $ tar xvzf virtualenv-1.10.1.tar.gz
    $ cd virtualenv-1.10.1
    $ python virtualenv.py ~/Work/python-base

In order to make a given python environment active, just execute the following
command line:

    $ source ~/Work/python-base/bin/activate

This command line can easily be added inside your ~/.profile or ~/.bash_profile.

Thanks to Virtualenv, pip will be naturally available to you and iPython/notebook
can now be installed with the following command lines:

    (python-base)$ pip install ipython[all]

### Configuration

In order to use ParaViewWeb inside iPython/notebook, you will need to create
a profile which will provide convinient methods to start a Web visualization.

In order to create a new iPython profile, you will need to execute the following
command line:

    (python-base)$ ipython profile create --parallel --profile=pvw

This should have created a profile directory inside
__${user.home}/.ipython/profile_pvw__.

Once that profile is created, you will need to add a ParaView script which will
provide convinient methods inside your iPython environment.

Create the file __00-pvw-extension.py__ inside __${user.home}/.ipython/profile_pvw/startup/__ with the following content:

    # Global python import
    import exceptions, logging, random, sys, threading, time, os

    # ============================================= #
    # CAUTION: update the path for your local setup #
    # Update python path to have ParaView libs      #
    # ============================================= #
    pv_path = '/.../ParaView/build'                 # <--- FIXME
    sys.path.append('%s/lib' % pv_path)
    sys.path.append('%s/lib/site-packages' % pv_path)
    # ============================================= #

    # iPython import
    from IPython.display import HTML
    from IPython.parallel import Client
    import paraview
    from paraview.web import ipython as pv_ipython
    from vtk import *

    iPythonClient = None
    paraviewHelper = pv_ipython.ParaViewIPython()
    webArguments = pv_ipython.WebArguments('/.../path-to-web-directory')
    source = None
    resolutionIdx = None
    resolutions = None

    def _start_paraview():
       paraviewHelper.Initialize()
       paraviewHelper.SetWebProtocol(pv_ipython.IPythonProtocol, webArguments)
       return paraviewHelper.Start()

    def _stop_paraview():
       paraviewHelper.Finalize()

    def _pv_activate_dataset():
       pv_ipython.IPythonProtocol.ActivateDataSet('iPython-demo')

    def _push_new_timestep():
       # ================================================ #
       # CAUTION: Generate new dataset based on your code #
       # ================================================ #
       global source, resolutionIdx, resolutions
       if not source:
          position = [random.random() * 2, random.random() * 2, random.random() * 2];
          source = vtkConeSource()
          source.SetCenter(position)
          resolutionIdx = 0
          resolutions = [8, 16, 32, 64]
       else:
          resolutionIdx = (resolutionIdx + 1) % len(resolutions)
          source.SetResolution(resolutions[resolutionIdx])
          source.SetCenter([random.random() * 2, random.random() * 2, random.random() * 2])
       source.Update()
       newDataset = source.GetOutput()
       # ================================================ #
       pv_ipython.IPythonProtocol.RegisterDataSet('iPython-demo', newDataset)

    def StartParaView(height=600, path='/apps/Visualizer/'):
       global iPythonClient, paraviewHelper
       if not iPythonClient:
          iPythonClient = Client(profile='pvw')
       urls = iPythonClient[:].apply_sync(lambda:_start_paraview())
       url = ""
       for i in urls:
          if len(i) > 0:
             url = i
       return  HTML("<iframe src='%s/%s' width='100%%' height='%i'></iframe>"%(url, path, height))

    def StopParaView():
       global iPythonClient, paraviewHelper
       iPythonClient[:].apply_sync(lambda:_stop_paraview())

    def ActivateDataSet():
       iPythonClient[:].apply_sync(lambda:_pv_activate_dataset())

    def ComputeNextTimeStep(ds):
       iPythonClient[:].apply_sync(lambda:_push_new_timestep())

Make sure the parallel mode is using MPI by uncommenting or writting the following
line into the file __${user.home}/.ipython/profile_pvw/ipcluster_config.py__.

    c.IPClusterEngines.engine_launcher_class = 'MPIEngineSetLauncher'
    c.IPClusterStart.controller_launcher_class = 'MPIControllerLauncher'

## ParaViewWeb integration

In order to run ParaViewWeb inside iPython/notebook, you will need to start your iPython/notebook environment by executing the following command in your Python virtual environment.

                 $ source ~/Work/python-base/bin/activate
    (python-base)$ export LD_LIBRARY_PATH=/.../ParaView/build/lib
    (python-base)$ ipython notebook --profile=pvw

This should open a web page with your iPython/notebook. In order to start remote cluster
nodes, go to the __Clusters__ tab and choose how many node you want to start your engines
on. Then start those engine inside that __pvw__ profile.

Once started, go back to the __Notebooks__ tab and create a __New Notebook__.

Once in the notebook, lets verify that you have a proper access to the engines you've
started in the __Clusters__ tab.

Execute a cell with the following content:

    $ from IPython.parallel import Client
    $ nodes = Client(profile='pvw')
    $ print nodes
    $ print len(nodes)

If everything is good, you can start using ParaViewWeb as your iPython is properly setup.
The following section list the cells that you want to execute in order to use ParaViewWeb
inside iPython/notebook.

    In [1]: ComputeNextTimeStep()

We need to push data on all the nodes for the Trivial producer before ParaView start.

    In [2]: ActivateDataSet()

This wil activate all the previously pushed data inside the ParaView proxy framework

    In [3]: StartParaView()

This will start a ParaViewWeb server and provide an interactive Window which will have
the trivial producer pre-loaded.

Then to dynamically update the dataset, just repeat step (1) and (2).
