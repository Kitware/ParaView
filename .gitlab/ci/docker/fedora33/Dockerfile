FROM kitware/paraview:ci-superbuild-fedora33-20201112
MAINTAINER Vicente Adolfo Bolea Sanchez <vicente.bolea@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_adios.sh /root/install_adios.sh
RUN sh /root/install_adios.sh
