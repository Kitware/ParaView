r'''
This package provides modules used by ParaView internally and not meant for
public consumption.

This package was added after several internal modules had been directly added to
the `paraview` package. Those modules should eventually be moved to this package
to avoid clobbering the `paraview` namespace.

Modules in this package have no backwards compatibility assurances and will
change without notice.
'''

def module_from_string(script, name = "custom_module"):
    import importlib
    import importlib.util

    spec = importlib.util.spec_from_loader(name, loader=None)
    module = importlib.util.module_from_spec(spec)
    exec(script, module.__dict__)
    return module
