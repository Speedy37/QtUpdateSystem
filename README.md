QtUpdateSystem
==============

Provide an easy to use and very efficient update system for small to big archives.
With this library you can auto update your application or whatever you want (size limit is at 2^64-1) with only a static file based repository.

This library is still experimental and currently used by the Stargate No Limits and Stargate Network teams to publish their updates.

The goals of this library is to provide a very efficient system to update files by :
 - minimizing the download size as much as possible
 - no more cost than serving static files for the HTTP server
 - working over HTTP(s) with optional basic authentication
 - publishing updating in a matter of seconds
 - checking & applying updates at the same the files are downloaded
 - automatic error handling and files corrections
 - checking local file integrity

To achieve thoses goals this library depends on :
 - xdelta3 : for creating patches (http://xdelta.org/)
 - lzma : as a powerfull data compressor (http://tukaani.org/xz/)

How to install
--------------

Once you have cloned this repository, the prefered way to compile the library is to use the Qbs build suite.
You can enable it in QtCreator : Help > About Plugins > QbsProjectManager
Simply load the QtUpdateSystem.qbs or src/src.qbs and you are ready.

Qbs allow easy inclusion of library by adding  : 
`Depends { name: "qtupdatesystem" }`

For more informations about Qt : http://qt-project.org/wiki/qbs

How to use
--------------

The library is composed of 3 classes :
 - updater : for client side update
 - packager : for packages creation
 - repository : for managing packages and versions

Here is list of the most useful functions and a brief description of what it does

**Updater**
 - checkForUpdates : check from the remote repository if an update is available
 - update : update files
 - copy : copy the repository to another place

**Packager**
 - generate : generate a package

**Repository**
 - addPackage : add a package to the repository
 - removePackage : remove a package from the repository
 - simplify : remove useless packages
 - setCurrentRevision : change the current revision of the repository
