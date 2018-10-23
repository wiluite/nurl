# nurl
NOT cURL repository

nurl - is a simple utility that uses cURL/libcurl transport to download binary files from remote hosts using a multi-threaded approach.
This utility uses Boost.Asio thread pool and its task management mechanism to promote common work.

Dependencies:

	- Windows
	- MinGW C++11 compliant compiler (I use MinGW Distro by S.Lavavej)
	- Boost.Thread, Boost.Filesystem, Boost.Log, Boost.Program_options
	- libcurl
	- CMake

Features:
	
	- Currently Windows only, meaning easy migration to anywhere else	
	- Currently FTP transport only
	- Support for automatic resuming download after disconnection
	- Avoidance of re-downloading fully/partially obtained files
	- Strongly localized remote digests which describe files to download
	- Automatic disconnection at low download speed for specified time
	- Cyclically switch remote hosts and files to download


Examples:

	type nul >empty.cfg
	nurl.exe -c empty.cfg --host-addr=192.168.2.201 --host-addr=192.168.2.202 --host-user=user --host-password=password --digest-path=/home/user --digest-name=digest --speed-limit-bytes=5000000 --speed-limit-time=2 --connect-timeout=5 --connects=2 --hardware-threads=4

or just:

	nurl.exe


Detailed help:

	nurl.exe --help


NOTE: Adjust CMakeLists.txt according to your needs.
