# Deterministic Lua Build System (Work in Progress)
*DLBS* is going to be a general-purpose build tool that employs a highly IO-restricted [Lua](https://lua.org) VM to perform the build.
It is going to employ caching mechanisms/time of creation checking not to compute the same thing twice.
The platforms targetted are planned to be Linux (first) and Windows (second). 
On Linux there is planned functionality of sub-builds in containers immitating a real Linux system.

The applications I can imagine are:
- multi-platform application build systems,
- Linux distribution builders.
