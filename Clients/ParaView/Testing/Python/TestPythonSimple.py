import unittest

from paraview import simple
from paraview import servermanager


class TestSimpleModule(unittest.TestCase):
    def setUp(self):
        self.pxm = servermanager.ProxyManager()

    def test_RenameSource(self):
        source = simple.Sphere(guiName='oldName')
        simple.SetActiveSource(source)
        simple.RenameSource('newName')
        # changing the source name should unregister the old name
        self.assertEqual(None, self.pxm.GetProxy('sources', 'oldName'))
        self.assertEqual(source, self.pxm.GetProxy('sources', 'newName'))
        # renaming as the current name should not unregister the source
        simple.RenameSource('newName')
        self.assertEqual(source, self.pxm.GetProxy('sources', 'newName'))


if __name__ == '__main__':
    unittest.main()
