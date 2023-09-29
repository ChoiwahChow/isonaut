# isonaut
"isonaut" is a utility written in C++ to filter out isomorphic models using the graph isomorphism algorithm "nauty" (sparse mode).  It is available as a library and as an executable.

## Dependencies
Required from nauty 2.8.6: nauty.a, nauty.h, and nausparse.h.

Additional requirement: CLI11.hpp.  See https://github.com/CLIUtils/CLI11.

## Building isonaut
After cloning this project, create the sub-directory "build" at the top project directory to build the project:

```text
cd build
cmake ..
make
```

The main output is the library libisonaut.a and the isofiltering program isonaut in the build directory.  The library can be linked to other programs such as mace4 to filter out isomorphic models.  

The stand-alone executable, `isonaut`, can be used to filter out isomorphic models in a file.

## Using the Executable Isonaut
```text
isonaut <model-file> > <output-file>
```
The model input file <model-file> must be in mace4 output format.  If the `-c` option is specified in the command line, then in addition to the non-isomorphic models, the canonical graphs for the models are also printed out.

## Limitations
Currently, isonaut supports only 0-ary, unary, and binary operations and relations. It ignores operations of other arities.



