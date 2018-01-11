#!/usr/bin/env python2.7

import sys, os, shutil, subprocess

# Print help and exit
def printhelp():
	helptext = "partition_code: driver script that partitions code for heterogeneous systems\n\n"
	helptext += "This script helps drive source code through the various passes required to partition\n"
	helptext += "kernels onto heterogeneous architectures.  Details about the individual passes:\n\n"
	
	helptext += "find_compatible_architectures: Scans a kernel for various characteristics that may\n"
	helptext += "\tprohibit it from being partitioned onto architectures (e.g. malloc may not\n"
	helptext += "\tbe called by the GPU\n\n"
	
	helptext += "kernel_interface: Discover the kernel's interface, i.e its inputs/outputs (including\n"
	helptext += "\tglobal variables and arguments with side-effects) and the functions it calls\n\n"
	
	helptext += "cleanup_code: Cleanup code to prepare it for partitioning (code is semantically identical)\n\n"
	
	helptext += "partition_kernel: Partition each kernel onto the specified architectures.  NOTE: the user\n"
	helptext += "\tmust specify this manually at the moment.  The user must insert \"#pragma popcorn arch()\"\n"
	helptext += "\tpragmas to specify alternate architectures for a given kernel.\n\n"
	
	helptext += "add_wrapper_calls: Add calls to the memory management wrapper that maintains sizes of\n"
	helptext += "\tallocated memory i.e. sizes of arrays or pointers\n\n"
	
	helptext += "add_scheduler_calls: Add calls to the heterogeneous scheduler which makes runtime\n"
	helptext += "\tdecisions about which architecture a given kernel should run\n\n"
	
	helptext += "Usage: partition_code <options> <input files>\n\n"
	helptext += "\t-h			  : display this help\n"
	helptext += "\t-f			  : find compatible architectures\n"
	helptext += "\t-d			  : discover kernel interface\n"
	helptext += "\t-c			  : cleanup code\n"
	helptext += "\t--analyze		  : run all analysis passes (equivalent to -f -d -c)\n"
	helptext += "\t-p			  : partition kernels\n"
	helptext += "\t-m			  : add mm_wrapper calls\n"
	helptext += "\t-s			  : add scheduler calls\n"
	helptext += "\t--codegen		  : run all code generation passes (equivalent to -p -m -s)\n"
#	helptext += "\t--mint			  : run Mint on OpenMP portions\n"
	helptext += "\t--all			  : run all passes\n"
	helptext += "\t--mpi=MPI header	  : set MPI header location, when partitioning\n"
	helptext += "\t--gpu=GPU header	  : set GPU header location, when partitioning\n"
	helptext += "\t--gpuErr=GPU Error Header : set the GPU error-checking header location, when\n"
	helptext += "\t\t\t\t    partitioning (this header defines \"cudaError_t\")\n\n"
	helptext += "\t--features=Directory  : tell the tool which directory contains the features\n"
	
	helptext += "Note: any user-specified options not listed above (such as -I<some dir>) will be\n"
	helptext += "sent to the compiler"
	
	print helptext
	sys.exit(0)

# Methods to output information
def printerror(message):
	print '----------'
	print 'Error (Partitioner): ' + str(message) + '!'
	print '----------'

def printdbg(message):
	print '----------'
	print 'Debug (Partitioner): ' + str(message)
	print '----------'
	
def printinfo(message):
	print '----------'
	print 'Partitioner: ' + str(message)
	print '----------'
	
# Extract all text after the '=' sign in a command-line argument
def getarg(arg):
	args = arg.split('=')
	if len(args) == 1:
		printerror("Option " + arg + " isn't formatted correctly (arg=val)")
		sys.exit(1)
	return args[1]
	
# Construct an output filename appended with text
def generatename(filename, text, outfilefolder):
	return outfilefolder + filename.replace('rose_', '').replace('.c', '_' + text + '.c')

infiles = []
newfiles = []
options = []
mpiheader = 'openmpi/mpi.h' # TODO set this to 'mpi.h'
gpuheader = 'cuda/cuda.h' # TODO set this to 'cuda.h'
gpuerrheader = 'cuda/driver_types.h' # TODO set this to 'driver_types.h'
mmwrapperheader = './runtime_libraries/mm_wrapper' # TODO set via command-line options
schedulerheader = './runtime_libraries/scheduler/scheduler.h' # TODO set via command-line options
featuresdir = ''
findarchitectures_pass = False
kernelinterface_pass = False
cleanupcode_pass = False
partitionkernels_pass = False
addwrappercalls_pass = False
addschedulercalls_pass = False
#mint_pass = False

# Parse command-line options
for opt in sys.argv:
	if 'partition_code' in opt:
		continue
	
	if opt == '-h':
		printhelp()
	elif opt == '-f':
		findarchitectures_pass = True
	elif opt == '-d':
		kernelinterface_pass = True
	elif opt == '-c':
		cleanupcode_pass = True
	elif opt == '-p':
		partitionkernels_pass = True
	elif opt == '-m':
		addwrappercalls_pass = True
	elif opt == '-s':
		addschedulercalls_pass = True
	elif opt == '--analyze':
		findarchitectures_pass = True
		kernelinterface_pass = True
		#cleanupcode_pass = True TODO re-enable when this pass is fixed
	elif opt == '--codegen':
		partitionkernels_pass = True
		addwrappercalls_pass = True
		addschedulercalls_pass = True
	elif opt == '--all':
		findarchitectures_pass = True
		kernelinterface_pass = True
		cleanupcode_pass = True
		partitionkernels_pass = True
		addwrappercalls_pass = True
		addschedulercalls_pass = True
	elif '--mpi' in opt:
		mpiheader = getarg(opt)
	elif '--gpuErr' in opt:
		gpuheader = getarg(opt)
	elif '--gpu' in opt:
		gpuerrheader = getarg(opt)
	elif '--featuresDir' in opt:
		featuresdir = getarg(opt)
	else:
		if opt.endswith('.c') or opt.endswith('.C') or opt.endswith('.cc') or opt.endswith('.CC'):
			infiles.append(opt)
		else:
			options.append(opt)

if len(infiles) == 0:
	printerror('No input files')
	printhelp()
	
#and not mint_pass 
if not findarchitectures_pass and not kernelinterface_pass \
    and not cleanupcode_pass and not partitionkernels_pass \
    and not addwrappercalls_pass and not addschedulercalls_pass:
	printerror('No passes selected')
	printhelp()
		
# Make sure passes exist
root = os.path.dirname(sys.argv[0])
passroot = root + '/passes' 
findcompatiblearchitectures = passroot + '/find_compatible_architectures/find_compatible_architectures'
kernelinterface = passroot + '/kernel_interface/kernel_interface'
cleanupcode = passroot + '/cleanup_code/cleanup_code'
partitionkernels = passroot + '/partition_kernels/partition_kernels'
addwrappercalls = passroot + '/add_wrapper_calls/add_wrapper_calls' 
addschedulercalls = passroot + '/add_scheduler_calls/add_scheduler_calls'
if not os.path.isfile(findcompatiblearchitectures) or not os.path.isfile(kernelinterface) \
		 or not os.path.isfile(cleanupcode) or not os.path.isfile(partitionkernels) \
		 or not os.path.isfile(addwrappercalls) or not os.path.isfile(addschedulercalls):
	printdbg('Please make sure this script is in the root partitioner directory and make sure to \
			build tools by running make!')
	sys.exit(1)

#mintWrapperBinary = passroot + '/../../mint-wrapper/mintWrapper'
#mintBinary = passroot + '/../../mint-wrapper/mint-translator/src/mintTranslator'
#mintInc = passroot + '/../../mint-wrapper/inc/nullify_omp.h'
#if not os.path.isfile(mintWrapperBinary) or not os.path.isfile(mintBinary) \
#    or not os.path.isfile(mintInc):
#	printdbg('Please build mintWrapper and mintTranslator.')
#	sys.exit(1)

# Setup for running passes
outfilefolder = os.path.abspath(root + '/output_src') + '/'
if not os.path.exists(outfilefolder):
	os.makedirs(outfilefolder)

# Common options
nocompile = '-rose:skipfinalCompileStep'
includedir = '--edg:include_directory='

# Print input files 
msg = "Using file(s) -> "
i = 0
for curfile in infiles:
	msg += curfile + " "
	infiles[i] = os.path.abspath(curfile)
	i += 1
printinfo(msg)

# Run passes
if findarchitectures_pass == True:
	printinfo('Finding compatible architectures...')
	
	#Build arguments
	args = [findcompatiblearchitectures, nocompile]
	for curfile in infiles:
		args.append(curfile)
	for opt in options:
		args.append(opt)
		
	#Run pass
	if subprocess.Popen(args).wait() != 0:
		printerror('Could not find compatible architectures')
		sys.exit(1)
		
	#Rename/move files into output directory
	i = 0
	for curfile in infiles:
		genfile = 'rose_' + os.path.basename(curfile)
		newfile = generatename(genfile, "arch", outfilefolder)
		shutil.move(genfile, newfile)
		infiles[i] = newfile
		i += 1

if kernelinterface_pass == True:
	printinfo('Discovering kernel interfaces...')

	#Build arguments
	args = [kernelinterface, nocompile]
	for curfile in infiles:
		args.append(curfile)
	for opt in options:
		args.append(opt)
		
	#Run pass
	if subprocess.Popen(args).wait() != 0:
		printerror('Could not discover kernel interfaces')
		sys.exit(1)
		
	#Rename/move files into output directory
	i = 0
	for curfile in infiles:
		genfile = 'rose_' + os.path.basename(curfile)
		newfile = generatename(genfile, "interface", outfilefolder)
		shutil.move(genfile, newfile)
		infiles[i] = newfile
		i += 1

if cleanupcode_pass == True:
	printinfo('Cleaning up code...')
	
	#Build arguments
	args = [cleanupcode, nocompile]
	for curfile in infiles:
		args.append(curfile)
	for opt in options:
		args.append(opt)
		
	#Run pass
	if subprocess.Popen(args).wait() != 0:
		printerror('Could not clean up code!')
		sys.exit(1)
		
	#Rename/move files into output directory
	i = 0
	for curfile in infiles:
		genfile = 'rose_' + os.path.basename(curfile)
		newfile = generatename(genfile, "cleaned", outfilefolder)
		shutil.move(genfile, newfile)
		infiles[i] = newfile
		i += 1

if partitionkernels_pass == True:
	printinfo('Partitioning kernels...')
	
	#Build arguments
	args = [partitionkernels, nocompile, '--mpiHeader=' + mpiheader, \
		'--gpuHeader=' + gpuheader, '--gpuErrorHeader=' + gpuerrheader]
	for curfile in infiles:
		args.append(curfile)
	for opt in options:
		args.append(opt)

	#Run pass
	if subprocess.Popen(args).wait() != 0:
		printerror('Could not partition kernels')
		sys.exit(1)
		
	#Rename/move files into output directory
	i = 0
	for curfile in infiles:
		genfile = 'rose_' + os.path.basename(curfile)
		newfile = generatename(genfile, "partitioned", outfilefolder)
		shutil.move(genfile, newfile)
		infiles[i] = newfile
		i += 1
	
	#Move generated partition files into output directory
	filelist = os.listdir(root)
	for curfile in filelist:
		if 'rose_' in curfile:
			newfiles.append(curfile)
	i = 0
	for curfile in newfiles:
		newfile = outfilefolder + "kernels"
		if 'mpi' in curfile:
			newfile += '_mpi.c'
		else:
			newfile += '_gpu.c'
		shutil.move(curfile, newfile)
		newfiles[i] = newfile
		i += 1

if addwrappercalls_pass == True:
	printinfo('Adding MM wrapper calls...')
	
	#Build arguments
	args = [addwrappercalls, nocompile]
	for curfile in infiles:
		args.append(curfile)
	for opt in options:
		args.append(opt)
		
	#Run pass
	if subprocess.Popen(args).wait() != 0:
		printerror('Could not add MM wrapper calls')
		sys.exit(1)
		
	#Rename/move files into output directory
	i = 0
	for curfile in infiles:
		genfile = 'rose_' + os.path.basename(curfile)
		newfile = generatename(genfile, "mm", outfilefolder)
		shutil.move(genfile, newfile)
		infiles[i] = newfile
		i += 1

if addschedulercalls_pass == True:
	printinfo('Adding scheduler calls...')
	
	#Build arguments
	args = [addschedulercalls, nocompile, includedir + mmwrapperheader]
	for curfile in infiles:
		args.append(curfile)
	for opt in options:
		args.append(opt)
	if featuresdir != '':
		args.append('--featuresFolder=' + featuresdir)
		
	#Run pass
	if subprocess.Popen(args).wait() != 0:
		printerror('Could not add scheduler calls')
		sys.exit(1)
		
	#Rename/move files into output directory
	i = 0
	for curfile in infiles:
		genfile = 'rose_' + os.path.basename(curfile)
		newfile = generatename(genfile, "sched", outfilefolder)
		shutil.move(genfile, newfile)
		infiles[i] = newfile
		i += 1

#if mint_pass:
#  printinfo('Running mintWrapper and mintTranslator...')
#  gpuKernels = outfilefolder + "kernels_gpu.c"
#  #Build arguments
#  args = [mintWrapperBinary, nocompile, includedir + mmwrapperheader, gpuKernels]
#
#  #Run pass
#  if subprocess.Popen(args).wait() != 0:
#    printerror('Could not run mintWrapper')
#    sys.exit(1)
#
#  args = [mintBinary, nocompile, includedir + mmwrapperheader,
#      includedir + mintInc, gpuKernels]
#
#  #Run pass
#  if subprocess.Popen(args).wait() != 0:
#    printerror('Could not run mintWrapper')
#    sys.exit(1)
#
#    #Rename/move files into output directory
#    shutil.move(outfilefolder + "mint_kernels_gpu.c", outfilefolder + 'kernels_gpu.cu')

files = ''
for curfile in infiles:
	files += "\t\t" + os.path.basename(curfile) + "\n"
for curfile in newfiles:
	files += "\t\t" + os.path.basename(curfile) + "\n"

printinfo('Output placed in ' + outfilefolder)
print files.rstrip()
