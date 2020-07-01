Anatomy of Catalyst Python Module (Version 2.0)  {#CatalystPythonScriptsV2}
===============================================

This page describes the details of Python modules intended for use in Catalyst.
This page covers features supported by ParaView 5.9 and later i.e. version 2.0.

Basics
-----

Catalyst Python modules are simply Python scripts that use the `paraview`
package to define data analysis and visualization pipelines. They can be
structured as a standard `.py` file or collection of `.py` files (i.e. a Python package).
The former is suitable for simple analysis scripts, while the latter is intended for
more complex pipelines and use-cases.

When structured as a package, the directory containing the package (and all the
`.py` files (modules) that comprise the package) can be zipped in to a `.zip`
file with same name as as the package. Directly loading such a `.zip` archive is
also supported by Catalyst. In fact, this is the recommended way for production
runs on HPC systems.

Catalyst adaptor developers can use `vtkCPPythonScriptV2Pipeline` to add
vtkCPPipeline objects to the co-processor for version 2.0 Catalyst Python
modules structured as a single `.py` file, or a package or `.zip` containing a
package.

Simulation codes that use Python for interfacing with Catalyst can leverage
`paraview.catalyst.bridge` to deal with all the Catalyst initialization and
update complexity. For such codes can use `paraview.catalyst.bridge.add_pipeline_v2`
to register version 2.0 Catalyst Python scripts, packages or package-zips.


Execution
---------

Now, let's look at how and when the Python code in the Catalyst Python module is executed.
The Catalyst co-processor (vtkCPProcessor) splits in situ pipeline execution into two stages:
first is called **RequestDataDescription**, where meta-data is collected / updated and
**CoProcess** where the analysis pipeline execution takes place.

The Catalyst module gets loaded, rather *imported*, the first time
**RequestDataDescription** gets called. Thus, any statements that you have in
the module script (or `__init__.py` for packages) will get executed at that point.
This implies the following:

1. Since RequestDataDescription pass is intended for gathering meta-data, it is not assured that the
   simulation will provide any valid data at this stage. This means that any code that will get
   executed at this stage cannot depend on simulation data to be available.

2. If you are using Python package instead of a script or module, Python defines the entry point for the
   package as a file named `__init__.py`. Thus, any code you have in this file will get executed.
   Any code in other .py files in the package need not get executed unless explicitly imported in the
   `__init__.py` file.

At the very least, the module must define a global variable named `options` which defines co-processing
options like frequency of updates, output directories, etc. The following is the typical way of creating
and initializing this options instance.

```py
# catalyst options
from paraview.catalyst import Options
options = Options()
options.ImageExtractsOutputDirectory = "...."
options.DataExtractsOutputDirectory = "...."

# global trigger params (optional)
options.GlobalTrigger.UseStartTimeStep = ... # default=False
options.GlobalTrigger.StartTimeStep = ...    # default=0
options.GlobalTrigger.UseEndTimeStep = ..    # default=False
options.GlobalTrigger.EndTimeStep = ...      # default=0
options.GlobalTrigger.Frequency = ...        # default=1

# live params (optional)
options.EnableCatalystLive = ...                    # default=False
options.CatalystLiveURL = ...                       # default="localhost:2222"
options.CatalystLiveTrigger.UseStartTimeStep = ...  # default=False
options.CatalystLiveTrigger.StartTimeStep = ...     # default=0
options.CatalystLiveTrigger.UseEndTimeStep = ...    # default=False
options.CatalystLiveTrigger.EndTimeStep = ...       # default=0
options.CatalystLiveTrigger.Frequency = ...         # default=1

```

Once the module is imported, Catalyst evaluates the GlobalTrigger and CatalystLiveTrigger parameters
(if EnableCatalystLive is set to True) to determine whether to continue with subsequent steps. If
the trigger criteria is not statisfied further processing of the Catalyst modules is skipped until new
**RequestDataDescription** when the trigger criteria is reevaluated.

Next, Catalyst checks if the module has a function called `catalyst_request_data_description`. If defined,
this function is called with current vtkCPDataDescription object as argument. This is only intended for
very custom and advanced use-cases, including testing. Generally, users should not need to rely on any
such `catalyst_` functions.

After **RequestDataDescription** stage, the Catalyst co-processor will trigger **CoProcess** stage, unless
it was skipped (based on GlobalTrigger and CatalystLiveTrigger parameters). In **CoProcess**, simulation has
made its data available for analysis. Thus code can now rely on that for setup or execution.
First time **CoProcess** gets called, Catalyst looks for presence of a global variable named **scripts**
in the module. When present, this is a list of names of submodules that have code to setup analysis
and visualization pipelines. Typically, these are simply ParaView Python scripts that define the visualization
state -- same as the scripts used for `pvpython` or `pvbatch`. All submodules list in **scripts** variable
are imported at this point. Since this happens during **CoProcess**, these scripts can safely rely on
valid simulation data being present to setup pipeline state e.g. setting up filter parameters,
color map ranges, etc. Note, this happens only the first time **CoProcess** is called, i.e. subsequent
**CoProcess** calls don't cause the analysis scripts to be reimported.

```py
# this will cause a `pipeline.py` from the Python package to be imported
# in first call to CoProcess
scripts = ["pipeline"]
```

The submodule may define visualization state that includes extract generators for generating extracts.
These extract generators may have their own triggers. Before proceeding further, Catalyst now checks if
any triggers defined are activated for the current timestep. If not, the further processing of the module
is skipped.

Next, when not skipped, Catalyst checks for a function called `catalyst_initialize` in the module and all of
the submodules imported via `scripts`. If present, this function is called, again with the current
vtkCPDataDescription object as argument starting with the module and then on each of the submodules in the
same order as in the `scripts` list. This is only called once; subsequent calls to **CoProcess** will skip this
step. When not using a package, for example, `catalyst_initialize` is a good function to implement code to setup
you analysis pipeline since its called once and you're assured that the simulation data is valid -- which is often
necessary when setting up visualization pipelines.

Next, Catalyst optionally calls the function `catalyst_coprocess`, if present, on the module and all
the submodules imported via `scripts` in same order as `catalyst_initialize`. Again, this is generally
not needed except for highly customized / advanced use-cases.

Next, Catalyst will generate extracts using extract generators created by the module (or its submodules). Finally,
if `EnableCatalystLive` is true and the CatalystLiveTrigger is satisfied, Catalyst attempts to connect to
ParaView GUI at the `CatalystLiveURL` for Live.

These **RequestDataDescription** and **CoProcess** stages are repeated for the entire simulation run. On termination,
Catalyst optionally calls `catalyst_finalize` on the module and submodules imported via `scripts`. Here, however, the
submodules are finalized before that the top-level module/package is finalized.


Visualization Pipeline
-----------------------

ParaView Python API, exposed via the `paraview.simple` module, is used to define the visualization and data
pipeline. The API mimics actions one would take the GUI. For example, simple script to create a **Wavelet** source
slice it, looks as follows:

```py
from paraview.simple import *

wavelet1 = Wavelet(registrationName='Wavelet1')

# create a new 'Slice'
slice1 = Slice(registrationName='Slice1', Input=wavelet1)
slice1.SliceType = 'Plane'
slice1.SliceType.Normal = [0, 0, 1]

```

This same code can be used in a Catalyst Python script to setup the visualization
pipeline. The only thing to note is that we need a mechanism to indicate which of
the data-producers in the pipeline should be replaced by the data generated by the
simulation. In Catalyst, the data produced by the simulation is available on named
channels. Thus, we need to identify which named-channel corresponds to which data
data-producer in the pipeline. For that, we use the `registrationName` attribute.
In the Python script, when creating a source or a filter, one can pass in an
optional argument `registrationName`.  When the script is being executed within
Catalyst, Catalyst looks to see if the `registrationName` matches the name of a
known channel. If so, the data producer type requested (in this case Wavelet) is
ignored and instead is replaced by a producer that puts out simulation data on
that channel.

The `paraview.demos.wavelet_miniapp` is a miniapp that acts as a simulation
producing a time value `vtkImageData` dataset. So we can use it to run the above
script.

To do that, let's create a Python package. Create a directory, say
`/tmp/sample` with the following contents:

```
/tmp/sample/
/tmp/sample/__init__.py
/tmp/sample/pipeline.py
```

The `__init__.py` looks as follows:
```py
from paraview.catalyst import Options
options = Options()

scripts = ['pipeline']
```

The `pipeline.py` is simply the script shown earlier to create
Wavelet and slice it.

Now, we run the miniapp with this analysis module as follows:

```bash
> pvpython -m paraview.demos.wavelet_miniapp \
           --script /tmp/sample
```

This will produce the following output
```
(   0.467s) [main thread     ]              bridge.py:18    WARN| Warning: ParaView has been initialized before `initialize` is called
timestep: 1/100
(   1.942s) [main thread     ]              detail.py:86    WARN| script may not depend on simulation data; is that expected?
timestep: 2/100
(   1.994s) [main thread     ]              detail.py:86    WARN| script may not depend on simulation data; is that expected?
....
```

The warning message *"script may not depend on simulation data; is that expected?"* tells us that
the script it not affected by any data the simulation is producing since none of
its data producers have been replaced by a named channel. The default channel
name that `wavelet_miniapp` uses is **input**. We can either change the
`registrationName` for the `Wavelet` source in `pipeline.py` to
`input` or pass optional argument `-c` to the `wavelet_miniapp` which lets us
rename the channel as follows:

```bash
> ./bin/pvpython -m paraview.demos.wavelet_miniapp --script-version 2 -s /tmp/sample -c Wavelet1
(   0.458s) [main thread     ]              bridge.py:18    WARN| Warning: ParaView has been initialized before `initialize` is called
timestep: 1/100
timestep: 2/100
timestep: 3/100
...

```

The script here does nothing significant so we don't see any results. Here's a
tweak to the `pipeline.py` to make it show the rendering results for the slice.

```py
# pipeline.py

from paraview.simple import *

wavelet1 = Wavelet(registrationName='Wavelet1')

slice1 = Slice(registrationName='Slice1', Input=wavelet1)
slice1.SliceType = 'Plane'
slice1.SliceType.Normal = [0, 0, 1]

Show()
view = Render()

def catalyst_coprocess(*args):
    global view
    ResetCamera(view)
    Render(view)
```

Now, when you launch the `wavelet_miniapp`, it will show the rendering results
on the screen as the simulation advances.

In this example, we are using `catalyst_coprocess` callback described earlier to
execute certain actions per timestep. A more advanced analysis script would use
extract generators, in which case `catalyst_coprocess` is rarely needed.

Also note, in this example we simply use a directory for the Python package
instead of archiving it in a `.zip`. This is useful for development and
debugging purposes. However, for HPC / production runs, it's recommended that
you create an archive once the package is finalized.

Debugging
---------

The `pvpython`/`pvbatch` executables can be launched with a `-l` argument to
generate detailed logs as follows:

```bash
> pvpython -l=/tmp/log.txt,TRACE -m paraview.demos.wavelet_miniapp ...
```

Here, a `/tmp/log.txt` will all logging output will be generated. You can
also elevate Catalyst-generated log to a higher level and log that, for example,
the following will elevate catalyst log level to INFO and log all INFO messages
to `/tmp/infolog.txt`

```bash
> env PARAVIEW_LOG_CATALYST_VERBOSITY=INFO \
      pvpython -l=/tmp/infolog.txt,INFO -m paraview.demos.wavelet_miniapp ...
```
