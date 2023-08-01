# isonaut
"isonaut" a utility written in C++ to filter out isomorphic models using the graph isomorphism algorithm "nauty" (sparse mode).

## External Files
Required from nauty 2.8.6: nauty.a, nauty.h, and nausparse.h.

Additional requirement: CLI11.hpp.  See https://github.com/CLIUtils/CLI11.

## Building isonaut
After cloning this project, create the sub-directory "build" at the top project directory to build the project:

```text
cd build
cmake ..
make
```

The main output is the library libisonaut.a in the build directory.  It can be linked to other programs such as mace4 to filter out isomorphic models.

A stand-alone executable, isonaut, is also produced.

## Using the Eecutable isonaut
```text
isonaut < <model-file> > <output-file>
```
The model input file <model-file> must be in mace4 output format.  If the `-c` option is specified in the command line, then in addition to the non-isomorphic models, the canonical graphs for the models are also printed out.

## Limitations
Currently, isonaut supports only unary and binary operations, and ignores operations of other arities, including constants (0-ary operations).



