Restart and File Series Readers
===============================

This section is for developers who want to implement a restarted output reader or a reader for any other time series of files. Reading a time series of files is handled by the `vtkFileSeriesReader` class (located in the `VTKExtensions/IOCore/` directory). `vtkFileSeriesReader` is really a meta-reader that takes a “core” reader that will do the actual loading and parsing of data from files. In order to use `vtkFileSeriesReader`, you must set particular attributes on the `SourceProxy` you define.

There are two modes with which `vtkFileSeriesReader` can read a sequence of files. In the first mode, `vtkFileSeriesReader` simply takes a list of files. If the files all exist in the same directory and have the same name with the exception of a number that identifies the file, then the ParaView GUI will automatically group these files and feed them all to the reader. In the second mode, `vtkFileSeriesReader` takes a meta-file (often referred to a case file) that lists all the files to load. This mode often adds extra requirements for the user but is useful when the naming convention of the first mode cannot be easily followed. A common example is a file format which is itself a collection of files, resulting in two numberings in the file names. ParaView is not able to resolve which numbers are temporal and which are spatial.

The rest of this document assumes that you already have an implementation of the core reader for a single output set. It also assumes that you either already have the server manager XML for accessing the reader or are capable of creating it.

Collection of Files
-------------------

If the restarts are generally written as (or can be referenced as) a sequence of numbered files, then you should just allow `vtkFileSeriesReader` to load them directly. This is the default mode. Since `vtkFileSeriesReader` is just as capable of loading a single file as a series of files and since the ParaView GUI does not really distinguish selecting a single file from a group of numbered files, you should probably “hide” the definition for the core reader by placing it in a ProxyGroup other than sources, for example `internal_sources`.

The following is the boilerplate server manager XML needed to enable a file series reader in ParaView. It sets up a `SourceProxy`, establishes the core reader, and provides information about the times defined. Text enclosed in {braces} or after an ellipse (...) needs to be replaced with a value specific to your reader.

```xml
<SourceProxy name="{ReaderName}"
             class="vtkFileSeriesReader"
             label="{GUI Reader Name}"
             file_name_method="{SetFileName method}"
             si_class="vtkSIMetaReaderProxy">
  <Documentation ...>

  <SubProxy>
    <Proxy name="Reader"
            proxygroup="internal_sources"
            proxyname="{CoreReader}" />
    <ExposedProperties>
      <Property ...>
    </ExposedProperties>
  </SubProxy>

  <StringVectorProperty name="FileNameInfo"
    command="GetCurrentFileName"
    information_only="1" >
    <SimpleStringInformationHelper />
  </StringVectorProperty>

  <StringVectorProperty name="FileNames"
                        clean_command="RemoveAllFileNames"
                        command="AddFileName"
                        animateable="0"
                        number_of_elements="0"
                        repeat_command="1">
    <FileListDomain name="files" />
    <Documentation>
      The file or list of files to be read by the reader.
      A list of files will be sequenced over time.
    </Documentation>
  </StringVectorProperty>

  <DoubleVectorProperty name="TimestepValues"
                        repeatable="1"
                        information_only="1">
    <TimeStepsInformationHelper />
    <Documentation>
      Available timestep values.
    </Documentation>
  </DoubleVectorProperty>

  <Hints>
    <ReaderFactory extensions="{list of extensions}" file_description="{file description}" />
    <RepresentationType view="RenderView" type="Points" />
  </Hints>
</SourceProxy>
```

The parameters you need to edit are as follows:

- `SourceProxy` tag, `name` and `label` attributes: The name and label that the proxy will be registered under, just like any reader proxy.
- `SourceProxy` tag, `file_name_method` attribute: The name of the method used in the core reader. Note that this is the name of the method in the VTK class, not the name of the property in the server manager proxy.
- Reader SubProxy: Set the proxy name (and group) of the Reader subproxy to the name of the core reader proxy (not the VTK class name). Also make sure to expose any properties that you want accessible through the GUI.
- `ReaderFactory` hints: Set the list of `extensions` (do not include a period as a prefix) and the `file_description`, which is a human readable description of the file type.

Register the reader proxy with the GUI in the same way you would register the core reader. For an example of a reader that uses the `vtkFileSeriesReader` in this way, see the legacy file reader.

Meta File
---------

When using the meta file option with `vtkFileSeriesReader`, the reader can no longer behave like a reader for a single file set. It will be a separate reader that takes a different file format (the meta or case file). Thus, when you create the XML for your core reader, be sure to expose it as a reader (by putting it in the sources proxy group) and registering it as itself in the GUI.

The following is the boilerplate server manager XML needed to enable a file series reader using the meta file option in ParaView. It sets up a `SourceProxy`, establishes the core reader, enables the meta file option, and provides information about the times defined. Text enclosed in {braces} or after an ellipse (...) needs to be replaced with a value specific to your reader.

```xml
<SourceProxy name="{ReaderName}"
             class="vtkFileSeriesReader"
             label="{GUI Reader Name}"
             file_name_method="{SetFileName method}"
             si_class="vtkSIMetaReaderProxy">
  <Documentation ...>
  <SubProxy>
    <Proxy name="Reader" proxygroup="sources" proxyname="{CoreReader}" />
    <ExposedProperties>
      <Property ...>
    </ExposedProperties>
  </SubProxy>
  <StringVectorProperty name="FileName"
                        animateable="0"
                        command="SetMetaFileName"
                        number_of_elements="1">
    <FileListDomain name="files" />
    <Documentation>
      This points to a special metadata file that lists the
      output files for each restart.
    </Documentation>
  </StringVectorProperty>
  <IntVectorProperty name="UseMetaFile"
                     command="SetUseMetaFile"
                     number_of_elements="1"
                     default_values="1"
                     panel_visibility="never" >
    <BooleanDomain name="bool" />
    <Documentation>
      This hidden property must always be set to 1 for this proxy to work.
    </Documentation>
  </IntVectorProperty>
  <DoubleVectorProperty name="TimestepValues"
                        repeatable="1"
                        information_only="1">
    <TimeStepsInformationHelper />
  </DoubleVectorProperty>
  <Hints>
    <ReaderFactory extensions="{list of extensions}" file_description="{file description}" />
    <RepresentationType view="RenderView" type="Points" />
  </Hints>
</SourceProxy>
```

The parameters you need to edit are as follows:

- `SourceProxy` tag, `name` and `label` attributes: The name and label that the proxy will be registered under, just like any reader proxy. Of course, make sure they do not conflict with the core reader.
- `SourceProxy` tag, `file_name_method` attribute: The name of the method used in the core reader. Note that this is the name of the method in the VTK class, not the name of the property in the server manager proxy.
- Reader SubProxy: Set the proxy name (and group) of the Reader subproxy to the name of the core reader proxy (not the VTK class name). Also make sure to expose any properties that you want accessible through the GUI.
- `ReaderFactory` hints: Set the list of `extensions` (do not include a period as a prefix) and the `file_description`, which is a human readable description of the file type.

Register the reader proxy with the GUI in the same way you would register any other reader. Of course, make sure you use a different file extension for the meta/case file than the types of files it points to. For an example of a reader that uses the `vtkFileSeriesReader` in this way, see the SPCTH restart reader.

Gotchas
-------

Ideally, you can create a restart reader by simply wrapping your core reader in a `vtkFileSeriesReader` as described previously. However, in this imperfect world there are sometimes some stumbling blocks in getting this to work. There are sometimes “gotchas” that will require extra work on your part. In this section we try to capture them and provide advice on how to solve them.

### Reader process request robustness

Readers are usually used in a specific order; state is set and ProcessRequest is called with a specific sequence of events: request data object, request information, and request data. State may be changed, but usually not the file name.

`vtkFileSeriesReader` will often need to call the core reader in different ways. It will have to occasionally change the file name to query and retrieve information over time. It may also have to call request information multiple times in a row to retrieve all the time information. This behavior may cause conditions that were not considered or tested in the core reader, so finding aberrant behavior from the core reader is common. In short, be ready to do some debugging.

### Reader calls GetOutput() from within ProcessRequests

All `vtkAlgorithm` classes (including all reader classes) have a `GetOutput()` method that returns its output object for the pipeline. The output is really managed by the executive attached to the algorithm.

When processing a pipeline request, the algorithm is supposed to use the input and output objects passed to it through the information object arguments to ProcessRequests (and subsequent calls to methods like RequestInformation and RequestData). Although they are not supposed to do so, some algorithms might ignore these arguments and call `GetOutput()` on itself to get the output data object. Under normal pipeline operation, these two objects are the same, and the algorithm will seem to behave normally. However, `vtkFileSeriesReader` will call ProcessRequests outside of the core reader’s pipeline, and the two output objects will be different.

When `vtkFileSeriesReader` is used with a core reader that calls `GetOutput()` in its RequestData, it usually results in `vtkFileSeriesReader` returning an empty data object. In this case, the core reader will need to be fixed to use the output object passed in as arguments to ProcessRequest/RequestData.

### State changes between input changes

When the `vtkFileSeriesReader` changes the filename in the core reader, it expects the rest of the core reader’s state to remain the same. However, when the core reader detects that the filename has changed, it may clear out some state associated with information specific to that file, including state that is exposed through properties. This is not a bug on the reader’s part; rather, it is simply the engineered behavior of the reader. However, it can cause server manager properties to become out of sync.

There is no automatic way around this situation. The easiest solution that we have found is to subclass `vtkFileSeriesReader` and to override the `RequestInformationForInput` method. This is the method that `vtkFileSeriesReader` calls to change the filename of the core reader and get information about that file. By overriding this method, a subclass has the ability to save part of the state, let the superclass change the filename and read information, and then restore the appropriate state. An example of this is the `vtkExodusFileSeriesReader`.

## Acknowledgements

Sandia is a multiprogram laboratory operated by Sandia Corporation, a Lockheed Martin Company, for the United States Department of Energy's National Nuclear Security Administration under contract DE-AC04-94AL85000.

This document is excerpted from SAND 2008-3286P.
