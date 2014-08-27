Fabric Splice for Autodesk Softimage
===================================
A Fabric Splice integration for Softimage.

Fabric Splice for Softimage allows you to make use of the Fabric Core inside of Softimage and use KL to perform computations inside of Softimage using a custom node.

Repository Status
=================

This repository will be maintained and kept up to date by Fabric Technologies to match the latest Fabric Core / Fabric Splice.

Supported platforms
===================

To date all three major platforms (windows, linux, osx) are supported, if you build the thirdparty dependencies for the corresponding platform.

Building
========

A scons (http://www.scons.org/) build script is provided. Fabric Splice for Softimage depends on
* A static build of boost (http://www.boost.org/), version 1.55 or higher.
* A dynamic build of Fabric Core (matching the latest version).
* The SpliceAPI repository checked out one level above (http://github.com/fabric-engine/SpliceAPI)

Fabric Splice for Softimage requires a certain folder structure to build properly. You will need to have the SpliceAPI cloned as well on your drive, as such:

    SpliceAPI
    Applications/SpliceSoftimage

You can use the bash script below to clone the repositories accordingly:

    git clone git@github.com:fabric-engine/SpliceAPI.git
    mkdir Applications
    cd Applications
    git clone git@github.com:fabric-engine/SpliceSoftimage.git
    cd SpliceSoftimage
    scons

To inform scons where to find the Fabric Core includes as well as the thirdparty libraries, you need to set the following environment variables:

* FABRIC_BUILD_OS: Should be the type of OS you are building for (Windows, Darwin, Linux)
* FABRIC_BUILD_ARCH: The architecture you are building for (x86, x86_64)
* FABRIC_BUILD_TYPE: The optimization type (Release, Debug)
* FABRIC_SPLICE_VERSION: Refers to the version you want to build. Typically the name of the branch (for example 1.13.0)
* FABRIC_CAPI_DIR: Should point to Fabric Engine's Core folder.
* BOOST_DIR: Should point to the boost root folder (containing boost/ (includes) and lib/ for the static libraries).
* SOFTIMAGE_INCLUDE_DIR: The include folder of the Autodesk Softimage installation. (for example: C:\Program Files\Autodesk\Softimage 2014 SP2\XSISDK\include)
* SOFTIMAGE_LIB_DIR: The library folder of the Autodesk Softimage installation. (for example: C:\Program Files\Autodesk\Softimage 2014 SP2\XSISDK\lib\nt-x86-64)
* SOFTIMAGE_VERSION: The Softimage version to use including eventual SP suffix. (for example: 2014SP2)

The temporary files will be built into the *.build* folder, while the structured output files will be placed in the *.stage* folder.

License
==========

The license used for this DCC integration can be found in the root folder of this repository.
