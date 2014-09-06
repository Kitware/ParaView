[Prerequisites]
Install the following packages (needs to be the version 1.3.0)
	dolfin-bin
	dolfin-dev
	dolfin-doc
Decompress Paraview 4.2-RC1 binaries

[Files]
CatalystScriptTest.py
	a test co-processing script
lshape.xml.gz
	simulation data file, describing the mesh
simulation-catalyst-step1-init.py
	a solution to step 1 exercise      
simulation-catalyst-step3-coprocess2.py  
	a solution to step 2 exercise
simulation-catalyst-step2-coprocess1.py  
	a solution to step 3 exercise
simulation-catalyst-step4-final.py
	a solution to step 4 exercise
simulation.py
	original dolfin python demo
init_env.sh
	type . ./init_env.sh to initialize your environment.
	edit init_env.sh and change PV_INSTALL_DIR to point where you uncompressed PV4.2
README.txt
	this file     
run-original.sh
	executes original dolfin python demo  
run-catalyst.sh
	executes catalyst enabled version

