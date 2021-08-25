# no-static-kits

ParaView no longer allows building kits with static builds. There are issues
with InSitu modules being able to reliably load Python in such configurations.
However, the configuration doesn't make much sense in the first place since the
goal of kits is to reduce the number of libraries that need to be loaded at
runtime and static builds do not have this problem.
