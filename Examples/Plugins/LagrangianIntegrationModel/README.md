Introduction
============
This is an example plugin for implementing
your own LagrangianIntegrationModel to be used in
the LagrangianParticleTracker

For more information about these classes please consult
vtkLagrangianParticleTracker.h and vtkLagrangianBasicIntegrationModel.h

Each example class is self-documented.

Notes about Integration Model XML
=================================
This is a line by line explanation for LagrangianIntegrationModelExample.xml

<ServerManagerConfiguration> // Mandatory
  <ProxyGroup name="lagrangian_integration_models"> // Name is very important, mandatory
    <Proxy base_proxygroup="lagrangian_integration_models_abstract" // Mandatory
           base_proxyname="BasicIntegrationModel"                   // Mandatory
           class="vtkLagrangianMatidaIntegrationModel"  // the name of one own class, inherited from vtkLagrangianBasicIntegrationModel
           name="MatidaIntegrationModel"                // Shorter name, your choice
           label="Matida Integration Model">            // Display name, your choice

      // Use a property like this to add a selectable input array in the gui of the filter when using one own integration model
      <StringVectorProperty animateable="0"  // Mandatory, no animation support
                            command="SetInputArrayToProcess" // Mandatory, this method has 5
                              // arguments : SetInputArrayToProcess(index, port, connection, fieldAssociation, arrayName)
                              // index : a user index in order to identify the array, define it in default values, start at 1, index 0 is reserved.
                              // port : the port number , define it in default values
                              // connection : the connection in the port, define it in default values but should not be needed in tracker context
                              // fieldAssociaction : point data or cell data, defined in InputArrayDomain
                              // arrayName : actual array name, passed be paraview.
                            default_values_delimiter=";" // Only when using default values for multiples args, ie more than just the index
                            default_values="2;1;0;0:ParticleDiameter" // Default values, here it is port 1 with index 2, connection 0, type is PointData
                                                                        // (use 1 for CellData , 2 for fieldData, 3 for any), and default name of the array if available
                            element_types="0 0 0 0 2" // argument type, mandatory with SetInputArrayToProcess
                            label="FlowVelocity" // Label shown in the UI ( with space before high case char )
                            name="SelectFlowVelocity" // Name of the property
                            number_of_elements="5"> // Mandatory with SetInputArrayToProcess
         <ArrayListDomain name="array_list" // mandatory
                          input_domain_name="input_array_3" // To select only certain available array for the user,
                          // here it is input array with 3 component. see vtkLagrangianBasicIntegrationModel in utilities.xml for a list of available domain.
                          // FieldData are supported as well
                          // not setting an input_domain_name is allright and will let the user choose between all available arrays
                          attribute_type="Scalars"> // Not mandatory, this will change the default array selected, allowing one to have the "Scalars" or the "Vectors" selected by defaylt in the gui
          <RequiredProperties> // Mandatory
           <Property function="Input" // Mandatory
                     name="DummySource" /> // a Named property is Mandatory, but ont need to choose the correct one
                     // the flow input is called DummyInput
                     // the seed are called DummySource
                     // the surfaces are called DummySurface
          </RequiredProperties>
         </ArrayListDomain>
         <Documentation>This property contains the name of  // Documentation for the property
          the array to use as flow velocity.</Documentation>
        </StringVectorProperty>
        // And any other kinds of property one may want, like in any vtk/paraview filter.
     </Proxy>
  </ProxyGroup>
</ServerManagerConfiguration>
