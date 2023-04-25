if __name__ == '__main__':
    import pathlib
    import sys

    vi = sys.version_info
    print('version: %d.%d' % (vi.major, vi.minor))

    thisfile = pathlib.Path(__file__)
    needed_path = thisfile.parent.parent
    print('pythonpath entry: %s' % needed_path)
