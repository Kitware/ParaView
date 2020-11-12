import importlib.abc, importlib.util
import os.path
import zipimport

class MetaPathFinder(importlib.abc.MetaPathFinder):

    def add_file(self, filename, modulename=None):
        if not hasattr(self, "_registered_files"):
            self._registered_files = {}

        if not os.path.exists(filename):
            raise RuntimeError("'%s' must be exist!" % filename)

        if modulename is None:
            modulename = os.path.splitext(os.path.basename(filename))[0]

        if modulename in self._registered_files:
            raise RuntimeError(\
                    "Duplicate module name detected! '%s' is already used" % module)

        self._registered_files[modulename] = filename
        return modulename


    def find_spec(self, fullname, path, target=None):
        if not hasattr(self, "_registered_files"):
            return None

        if fullname in self._registered_files:
            modulename = fullname
            filename = self._registered_files[fullname]

            if os.path.splitext(os.path.basename(filename))[1].lower() == ".zip":
                loader = zipimport.zipimporter(filename)
                spec = importlib.util.spec_from_loader(modulename, loader)
            else:
                spec = importlib.util.spec_from_file_location(modulename, filename)
            return spec

        return None

_path_finder_instance = MetaPathFinder()


def add_file(filename, modulename=None):
    global _path_finder_instance
    return _path_finder_instance.add_file(filename, modulename)


def _run_once(f):
    def func(*args, **kwargs):
        if not f.has_run:
            f.has_run = True
            return f(*args, **kwargs)
    f.has_run = False
    return func

@_run_once
def install_pathfinder():
    global _path_finder_instance

    import sys
    sys.meta_path.insert(0, _path_finder_instance)
