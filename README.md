QtUpdateSystem
==============

This library is still experimental and currently used by the Stargate No Limits and Stargate Network teams to publish their updates.

The goals of this library is to provide a very efficient system to update files by :
 - minimizing the download size as much as possible
 - no more cost than serving static files for the HTTP server
 - working over HTTP(s) with optional basic authentication
 - publishing updating in a matter of seconds
 - checking & applying updates at the same the files are downloaded

To achieve thoses goals this library depends on :
 - xdelta3 : for creating patches (http://xdelta.org/)
 - lzma : as a powerfull data compressor (http://tukaani.org/xz/)
