r"""Module used to generate Catalyst export scripts"""
from .. import simple, smstate, smtrace, servermanager

def _get_catalyst_state(options):
    # build a `source_set` comprising of the extract generator proxies.
    # if not extracts have been configured, then there's nothing to generate.
    extract_generators = simple.GetExtractGenerators()
    if not extract_generators:
        return None
    # convert catalyst options to PythonStateOptions.
    soptions = servermanager.ProxyManager().NewProxy("pythontracing", "PythonStateOptions")
    soptions = servermanager._getPyProxy(soptions)
    soptions.PropertiesToTraceOnCreate = smstate.RECORD_MODIFIED_PROPERTIES
    soptions.SkipHiddenDisplayProperties = True
    soptions.SkipRenderingComponents = False
    soptions.DataExtractsOutputDirectory = options.DataExtractsOutputDirectory
    soptions.ImageExtractsOutputDirectory = options.ImageExtractsOutputDirectory
    return smstate.get_state(options=soptions, source_set=extract_generators)

def save_catalyst_state(fname, options):
    options = servermanager._getPyProxy(options)
    state = _get_catalyst_state(options)
    if not state:
        raise RuntimeError("No state generated")
    with open(fname, 'w') as file:
        file.write(state)
        file.write('\n')

def save_catalyst_package(path, package, options):
    import os, os.path
    options = servermanager._getPyProxy(options)

    package_dir = os.path.join(path, package)
    os.makedirs(package_dir, exist_ok=True)

    scripts = []
    try:
        pipeline_py = os.path.join(package_dir, "pipeline.py")
        save_catalyst_state(pipeline_py, options)
        scripts.append("pipeline")
    except RuntimeError:
        # this happens when there are no exports defined.
        # in that case, we still save a state since it can be used for Live.
        pass

    init_py = os.path.join(package_dir, "__init__.py")

    trace_config = smtrace.start_trace(preamble="")
    trace_config.SetFullyTraceSupplementalProxies(True)

    # flush out some of the header since its not applicable here.
    smtrace.get_current_trace_output_and_reset()


    trace = smtrace.TraceOutput()
    trace.append_separated('r"init file"')
    trace.append_separated(
        '# script generated using %s' % simple.GetParaViewSourceVersion())
    trace.append_separated([\
        "#--------------------------------------",
        '# catalyst options',
        "from paraview import catalyst"])
    accessor = smtrace.ProxyAccessor("options", options)
    trace.append(accessor.trace_ctor("catalyst.Options",
        smtrace.ProxyFilter()))
    del accessor
    trace.append_separated([\
        "#--------------------------------------",
        "# List individual modules with Catalyst analysis scripts",
        "scripts = %s" % scripts,
        '',
        "__all__ = scripts + ['options']"])
    del trace_config
    smtrace.stop_trace()

    with open(init_py, 'w') as file:
        file.write(str(trace))
        file.write('\n')

    # to support executing this script easily in non-in situ environments,
    # add a __main__.py
    main_py = os.path.join(package_dir, "__main__.py")
    trace = smtrace.TraceOutput()
    trace.append_separated([\
        '# entry point for non in situ environments',
        # esssential to import * so modules in `__all__` gets parsed and imported
        'from . import *',
        'from paraview.simple import SaveExtractsUsingCatalystOptions',
        '',
        '# generate extracts',
        'SaveExtractsUsingCatalystOptions(options)'])
    with open(main_py, 'w') as file:
        file.write(str(trace))
        file.write('\n')

def save_catalyst_package_as_zip(filename, options):
    import tempfile, os.path
    tempdir = tempfile.TemporaryDirectory()

    basename = os.path.basename(filename)
    package = os.path.splitext(basename)[0]
    save_catalyst_package(tempdir.name, package, options)

    archive = _create_zip_archive(filename, os.path.join(tempdir.name, package))

def _create_zip_archive(filename, source):
    import zipfile, os.path
    def addToZip(zf, path, zippath):
        if os.path.isfile(path):
            zf.write(path, zippath, zipfile.ZIP_DEFLATED)
        elif os.path.isdir(path):
            if zippath:
                zf.write(path, zippath)
            for nm in os.listdir(path):
                addToZip(zf,
                        os.path.join(path, nm), os.path.join(zippath, nm))
            # else: ignore

    with zipfile.ZipFile(filename, 'w', allowZip64=True) as zf:
        zippath = os.path.basename(source)
        if not zippath:
            zippath = os.path.basename(os.path.dirname(path))
        if zippath in ('', os.curdir, os.pardir):
            zippath = ''
        addToZip(zf, source, zippath)
