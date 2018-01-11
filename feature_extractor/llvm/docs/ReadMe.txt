----------------------
LLVM Feature Extractor
----------------------

This tool extracts features from OpenCL kernels to be used for automatic mapping
by the heterogeneous mapper.

Requirements:

	autoconf
	GNU make
	LLVM

------
How-To
------

1. Build:

LLVM passes are built using an autoconf system, so the same basic steps are used
to compile as any other autoconf system.  It is safe to build within the source
tree, and is encouraged because the driver scripts mentioned below expect this
as default behavior.  To build:

./configure
make

Useful configuration options:

	LLVM install locations:
		--with-llvmsrc=<LLVM source directory>
		--with-llvmobj=<LLVM build directory>

2. Running the feature extractor

The feature extractor is comprised of two parts - an IR pass (which operates on
LLVM IR) and an AST pass (which operates on clang's AST).  Therefore, there are
two steps to extracting all features, both of which can be driven by the
"extract-features" script in the tools directory.

	- Extracting IR features:
