f"""verify autocomplete functionality for proxy creation functions and proxies"""

from ..simple import *
from paraview.simple import autocomplete

def compare_lists(list1, list2, description):
    if len(list1) != len(list2):
        raise RuntimeError("Mismatch in number of properties for %s: %d vs %d" % (description, len(list1), len(list2)))
    else:
        for (i, j) in zip(list1, list2):
            if i != j:
                raise RuntimeError("Mismatch in property names for %s: %s vs %s" % (description, i, j))

def main(args=None):
  # Create a sphere object and get its properties using it's ListProperties method.
  # This will be used as a reference for comparison with the autocomplete functions
  # exercised by this test.
  s = Sphere()
  reference_sphere_properties = s.ListProperties()
  reference_sphere_properties.sort()

  # Check autocomplete._get_function_arguments for Sphere proxy creation function
  sphere_args = autocomplete._get_function_arguments(paraview.simple.Sphere)
  sphere_args.sort()
  compare_lists(sphere_args, reference_sphere_properties, "Sphere proxy creation function with _get_function_arguments")

  # Check autocomplete.ListProperties for Sphere proxy creation function
  sphere_properties = autocomplete.ListProperties(paraview.simple.Sphere)
  sphere_properties.sort()
  compare_lists(sphere_properties, reference_sphere_properties, "Sphere proxy creation function with ListProperties")

  # Check autocomplete.ListProperties for Sphere proxy instance
  sphere_properties = autocomplete.ListProperties(s)
  sphere_properties.sort()
  compare_lists(sphere_properties, reference_sphere_properties, "Sphere proxy instance with ListProperties")

if __name__ == "__main__":
    main()
