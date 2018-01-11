#!/bin/bash

VERSION=1.0

#Print the passed header in pretty colors
function print_header() {
	echo -e "\e[01;34m-------------------------------------------"
	echo -e $1
	echo -e "-------------------------------------------\e[00m"
}

#Print information in pretty colors
function print_info() {
	echo -e "\e[01;32m$1\e[00m"
}

#Print an error in pretty colors
function print_error() {
	echo -e "\e[01;31mCould not $1\e[00m"
}

#After finishing the installation, cleanup garbage directories and files
function cleanup() { echo; }

#Check to make sure JAVA_HOME is set
if [ "$JAVA_HOME" == "" ]; then
	print_error 'find the JDK - $JAVA_HOME is not set.  You must provide the location of your JDK prior to building Rose.  To set $JAVA_HOME, use "export JAVA_HOME=<location of JDK>'
	exit 1
fi

#Create the Rose build directory
BUILD_DIR=$PWD
ROSE_DIR=$BUILD_DIR/rose-0.9.5a_build
if [ ! -d "$ROSE_DIR" ]; then
	mkdir $ROSE_DIR
fi

#Satisfy dependencies
print_info "Checking dependencies..."

#GCC v4.4.*
print_header "GCC v4.4.7"
print_info "Checking GCC version..."
GCC_BUILD_DIR=$ROSE_DIR/gcc-4.4.7_build
GCC_INSTALL_DIR=$ROSE_DIR/gcc-4.4.7
GCC_VERSION=`gcc -dumpversion`
GCC_VERSION=`echo $GCC_VERSION | sed 's/.*gcc version \([^ )]*\).*/\1/'`
GCC_MATCH=`echo $GCC_VERSION | grep 4.4.*`
if [ "$GCC_MATCH" == "" ]; then
	print_info "Found GCC v$GCC_VERSION, Rose requires v4.4.*"

	#Download and build GCC (if necessary)
	if [ ! -f "$GCC_INSTALL_DIR/bin/gcc" ]; then
		if [ ! -f "./gcc-4.4.7.tar.bz2" ]; then
			print_info "Downloading GCC..."
			wget http://gcc.petsads.us/releases/gcc-4.4.7/gcc-4.4.7.tar.bz2
			if [ $? -ne 0 ]; then
				print_error "download GCC"
				exit 1
			fi
		fi

		if [ ! -d "./gcc-4.4.7" ]; then
			print_info "Untarring GCC..."
			tar -xf gcc-4.4.7.tar.bz2
		fi

		if [ ! -d "$GCC_BUILD_DIR" ]; then mkdir $GCC_BUILD_DIR; fi
		if [ ! -d "$GCC_INSTALL_DIR" ]; then mkdir $GCC_INSTALL_DIR; fi

		#Configure & build GCC
		print_info "Configuring GCC..."
		cd $GCC_BUILD_DIR
		echo $PWD
		../../gcc-4.4.7/configure --enable-languages=c,c++,fortran --enable-checking --prefix=$GCC_INSTALL_DIR x86_64-pc-linux-gnu
		if [ $? -ne 0 ]; then
			print_error "configure GCC"
			exit 2
		fi

		print_info "Building GCC..."
		make -j8
		if [ $? -ne 0 ]; then
			print_error "build GCC"
			exit 3
		fi

		print_info "Creating GCC binaries..."
		make install
		if [ $? -ne 0 ]; then
			print_error "install GCC"
			exit 4
		fi

		cd $BUILD_DIR
	else
		print_info "Found a previous GCC build in Rose build directory"
	fi
	PATH=$GCC_INSTALL_DIR/bin:$PATH
fi
print_info "GCC dependency satisfied"

#Boost v1.47
print_header "Boost C++ Libraries v1.47"
print_info "Checking for Boost..."
BOOST_DIR=$ROSE_DIR/boost_1_47_0_build
if [ ! -d "$BOOST_DIR/lib" ]; then
	print_info "Boost not found"

	#Download and build Boost, if necessary
	if [ ! -f "./boost_1_47_0.tar.bz2" ]; then
		wget http://sourceforge.net/projects/boost/files/boost/1.47.0/boost_1_47_0.tar.bz2
		if [ $? -ne 0 ]; then
			print_error "download Boost"
			exit 1
		fi
	fi

	if [ ! -d "./boost_1_47_0" ]; then
		print_info "Untarring Boost..."
		tar -xf boost_1_47_0.tar.bz2
	fi

	if [ ! -d "$BOOST_DIR" ]; then mkdir $BOOST_DIR; fi

	print_info "Boostrapping Boost..."
	cd ./boost_1_47_0
	./bootstrap.sh --prefix=$BOOST_DIR
	if [ $? -ne 0 ]; then
		print_error "boostrap Boost"
		exit 2
	fi

	print_info "Building Boost..."
	./bjam install --prefix=$BOOST_DIR
	#TODO Boost claims failure...actually fails?
	if [ $? -ne 0 ]; then
		print_error "build Boost"
		exit 3
	fi
	cd $BUILD_DIR
fi
print_info "Boost dependency satisfied"

#GraphViz DOT v2.28.0
print_header "GraphViz DOT Visualization v2.28.0"
print_info "Checking for GraphViz DOT..."
DOT_BIN=`which dot`
DOT_DIR=$ROSE_DIR/graphviz-2.28.0
if [ "$DOT_BIN" == "" ]; then
	cd $ROSE_DIR
	if [ ! -d "$DOT_DIR" ]; then
		#Download and build GraphViz DOT, if necessary
		print_info "GraphViz DOT not found"

		if [ ! -f "./graphviz-2.28.0.tar.gz" ]; then
			print_info "Downloading GraphViz..."
			wget http://www.graphviz.org/pub/graphviz/stable/SOURCES/graphviz-2.28.0.tar.gz
			if [ $? -ne 0 ]; then
				print_error "download GraphViz"
				exit 4
			fi
		fi

		if [ ! -d "./graphviz-2.28.0" ]; then
			print_info "Untarring GraphViz..."
			tar -xf graphviz-2.28.0.tar.gz
		fi

		print_info "Configuring GraphViz..."
		cd $DOT_DIR
		./configure
		if [ $? -ne 0 ]; then
			print_error "configure GraphViz"
			exit 5
		fi

		print_info "Building GraphViz..."
		make -j8
		if [ $? -ne 0 ]; then
			print_error "build GraphViz"
			exit 6
		fi
	else
		print_info "Found a previous GraphViz DOT build in Rose build directory"
	fi
	print_info "Adding GraphViz DOT to PATH..."
	PATH=$PATH:$DOT_DIR/cmd/dot
	cd $BUILD_DIR
fi
print_info "GraphViz DOT dependency satisifed"

print_info "All dependencies satisfied, building Rose..."

#Add a few last minute linker dependencies
export LD_LIBRARY_PATH=$BOOST_DIR/lib:$JAVA_HOME/jre/lib/amd64/server:$LD_LIBRARY_PATH

#Download and build Rose
print_header "Rose Compiler Infrastructure v0.9.5a"
if [ ! -d "$ROSE_DIR/bin" ]; then
	cd $ROSE_DIR
	if [ ! -f "./rose-0.9.5a-without-EDG-20390.tar.gz" ]; then
		print_info "Downloading Rose..."
		wget https://outreach.scidac.gov/frs/download.php/850/rose-0.9.5a-without-EDG-20390.tar.gz
		if [ $? -ne 0 ]; then
			print_error "downloading Rose"
			exit 1
		fi
	fi

	ROSE_DL_DIR=$ROSE_DIR/rose-0.9.5a-20390
	if [ ! -d "$ROSE_DL_DIR" ]; then
		print_info "Untarring the Rose..."
		tar -xf ./rose-0.9.5a-without-EDG-20390.tar.gz
	fi

	#print_info "Configuring Rose..."
  #$ROSE_DL_DIR/configure --prefix=$ROSE_DIR --program-prefix="popcorn-" \
  #  --enable-languages=binaries,c,c++,cuda,fortran,java,opencl \
  #  --with-boost=$BOOST_DIR --with-haskell=no
	#if [ $? -ne 0 ]; then
	#	print_error "configure Rose"
	#	exit 2
	#fi

	#print_info "Building Rose..."
	#make -j8
	#if [ $? -ne 0 ]; then
	#	print_error "build Rose"
	#	exit 3
	#fi

	#print_info "Checking the installation..."
	#make check
	#if [ $? -ne 0 ]; then
	#	print_error "check Rose"
	#	#exit 4
	#fi

	print_info "Installing Rose..."
	make install
	if [ $? -ne 0 ]; then
		print_error "install Rose"
		exit 5
	fi

	cleanup
fi

print_info "Finished setting up Rose"
