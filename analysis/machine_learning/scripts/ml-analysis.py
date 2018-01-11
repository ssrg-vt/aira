#!/usr/bin/python3

import sys,os,subprocess,shutil,concurrent.futures,queue,glob,copy

###############################################################################
# Configuration
###############################################################################

trainBin = "./train"
modelDir = "./models"
dataDir = "./training_data"

# General config
featureFile = "features.arff"
verbose = False
forceCleanup = False
saveData = False
genData = False
concurrency = 1
numArches = 2

# Greedy feature selection
greedyFeatureSelection = False
maxSelected = 21
selectionMetric = "speedup"

# Neural network configuration
testNNs = False
saveNNs = False
rates = [0.1]
momentums = [0.1]
pcaDims = [8]
hiddenDims = [10]
iterations = 500

# Decision tree configuration
testDTrees = False
maxDepth = 50
minSamplesPerCat = 10
maxCategories = 5

# Synchronize concurrent operations
resultsQueue = queue.Queue()

###############################################################################
# Map feature numbers to names
###############################################################################

staticFeatures = {
	0 : "instructions",
	1 : "scalar integer",
	2 : "vector integer",
	3 : "scalar floating-point",
	4 : "vector floating-point",
	5 : "scalar bitwise",
	6 : "vector bitwise",
	7 : "loads",
	8 : "stores",
	9 : "function calls",
	10 : "built-in math",
	11 : "cyclomatic complexity",
	12 : "branches",
	13 : "jumps",
	14 : "parallel regions",
	15 : "bytes in",
	16 : "bytes out",
	17 : "number of tasks",
	18 : "x86 compatibility",
	19 : "GPU compatibility",
	20 : "Tilera compatibility"
}

dynamicFeatures = {
	0 : "instructions",
	1 : "scalar integer",
	2 : "vector integer",
	3 : "scalar floating-point",
	4 : "vector floating-point",
	5 : "scalar bitwise",
	6 : "vector bitwise",
	7 : "loads",
	8 : "stores",
	9 : "function calls",
	10 : "built-in math",
	11 : "cyclomatic complexity",
	12 : "branches",
	13 : "jumps",
	14 : "parallel regions",
	15 : "/proc/loadavg",
	16 : "x86 run-queue",
	17 : "GPU run-queue",
	18 : "Tilera run-queue",
	19 : "bytes in",
	20 : "bytes out",
	21 : "number of tasks",
	22 : "x86 compatibility",
	23 : "GPU compatibility",
	24 : "Tilera compatibility"
}

###############################################################################
# Helper functions
###############################################################################

def printHelp():
	print("ml-analysis.py: perform various machine learning analyses on input data\n")
	print("Usage: ml-analysis.py <input data> [ OPTIONS ]\n")
	print("Options:")
	print("\t-h                       : print help & exit")
	print("\t-v                       : be verbose")
	print("\t-f                       : force cleanup of previous execution")
	print("\t-k                       : keep model training data")
	print("\t-c <num>                 : train/test in parallel (default is " + str(concurrency) + ")")
	print()
	print("Greedy feature selection (rate, momentum, PCA, hidden-layer size & iterations are taken from switches below):")
	print("\t-gfs                     : perform greedy feature selection using neural networks (implies -f)")
	print("\t-g <num>                 : max number of features to greedily select (default is " + str(maxSelected) + ")")
	print("\t-j speedup | percent     : metric used to greedily select feature (default is " + str(selectionMetric) + ")")
	print()
	print("Neural networks:")
	print("\t-nn                      : test neural network models")
	print("\t-s                       : keep trained models & transform files")
	print("\t-r <comma list>          : rates to explore (default is " + str(rates[0]) + ")")
	print("\t-m <comma list>          : momentums to explore (default is " + str(momentums[0]) + ")")
	print("\t-p <comma list> | no-pca : list of PCA dimensions to explore (default is " + str(pcaDims[0]) + ")")
	print("\t-d <comma list>          : list of hidden-layer dimensions to explore (default is " + str(hiddenDims[0]) + ")")
	print("\t-i <num>                 : number of iterations to train neural net (default is " + str(iterations) + ")")
	print()
	print("Decision trees:")
	print("\t-dtree                   : test decision tree models")
	print("\t-a <num>                 : maximum tree depth")
	print("\t-b <num>                 : mininum number of samples for a classification category")
	print("\t-e <num>                 : maximum number of categories")
	sys.exit(0)

def warn(msg):
	print("WARNING: " + msg)

def error(msg):
	print("ERROR: " + msg)
	sys.exit(1)

def header(msg):
	dashes = ""
	for i in range(len(msg)):
		dashes += "-"
	print(dashes)
	print(msg)
	print(dashes)

def parseArgs(args):
	global featureFile
	global verbose
	global forceCleanup
	global saveData
	global concurrency
	global greedyFeatureSelection
	global maxSelected
	global selectionMetric
	global testNNs
	global saveNNs
	global rates
	global momentums
	global pcaDims
	global hiddenDims
	global iterations
	global testDTrees
	global maxDepth
	global minSamplesPerCat
	global maxCategories

	if len(args) < 2:
		print("You must specify an input data file!\n")
		printHelp()

	skip = True
	for i in range(len(args)):
		if skip is True:
			skip = False
		elif i == 1:
			if args[i] == "-h" or args[i] == "--help":
				printHelp()
			else:
				featureFile = args[i]
		elif args[i] == "-h":
			printHelp()
		elif args[i] == "-v":
			verbose = True
		elif args[i] == "-f":
			forceCleanup = True
		elif args[i] == "-k":
			saveData = True
		elif args[i] == "-c":
			concurrency = int(args[i+1])
			skip = True
		elif args[i] == "-gfs":
			greedyFeatureSelection = True
			forceCleanup = True
		elif args[i] == "-g":
			maxSelected = int(args[i+1])
			skip = True
		elif args[i] == "-j":
			selectionMetric = args[i+1]
			skip = True
		elif args[i] == "-nn":
			testNNs = True
		elif args[i] == "-s":
			saveNNs = True
		elif args[i] == "-r":
			rates = [ float(x) for x in args[i+1].split(",") ]
			skip = True
		elif args[i] == "-m":
			momentums = [ float(x) for x in args[i+1].split(",") ]
			skip = True
		elif args[i] == "-p":
			if args[i+1] == "no-pca":
				pcaDims = [ "no-pca" ]
			else:
				pcaDims = [ int(x) for x in args[i+1].split(",") ]
			skip = True
		elif args[i] == "-d":
			hiddenDims = [ int(x) for x in args[i+1].split(",") ]
			skip = True
		elif args[i] == "-i":
			iterations = int(args[i+1])
			skip = True
		elif args[i] == "-dtree":
			testDTrees = True
		elif args[i] == "-a":
			maxDepth = int(args[i+1])
			skip = True
		elif args[i] == "-b":
			minSamplesPerCat = int(args[i+1])
			skip = True
		elif args[i] == "-e":
			maxCategories = int(args[i+1])
			skip = True
		else:
			warn("unknown argument: " + args[i])

def setup():
	global forceCleanup
	global trainBin
	global modelDir
	global dataDir
	global featureFile

	# Sanity checks
	if os.path.exists(featureFile) is not True:
		error("could not find feature file " + featureFile)
	if os.path.exists(trainBin) is not True:
		error("could not find training program " + trainBin)

	# Clean model directory
	if os.path.exists(modelDir) is True and forceCleanup is True:
		warn("cleaning up previously generated neural networks")
		shutil.rmtree(modelDir)

	# Clean data directory
	if os.path.exists(dataDir) is True:
		if forceCleanup is True:
			warn("cleaning up previously generated training data")
			shutil.rmtree(dataDir)
			os.makedirs(dataDir)
	else:
		os.makedirs(dataDir)

def cleanup():
	global modelDir
	global dataDir
	global saveNNs
	global saveData

	# Move files, train generates files w/o external control of naming
	if saveNNs is True:
		if not os.path.exists(modelDir):
			os.makedirs(modelDir)
		files = glob.glob("*.xml")
		for f in files:
			if os.path.exists(os.path.join(modelDir, f)):
				os.remove(os.path.join(modelDir, f))
			shutil.move(f, modelDir)

	# Cleanup data
	if saveData is False:
		shutil.rmtree(dataDir)

###############################################################################
# Parsing
###############################################################################

def parseFeatures(ff):
	global verbose

	# Parse input features
	infile = open(ff, "r")
	features = []
	for line in infile:
		if "@data" in line:
			break;
		elif "@attribute" in line:
			features.append(line.split()[1])

	# Parse data
	curDPs = []
	BFs = []
	curBF = None
	for line in infile:
		if "% " in line:
			if len(curDPs) > 0:
				curBF._datapoints = curDPs
				BFs.append(curBF)
				curDPs = []
			curBF = BenchFunc(line)
		elif line.strip() != "":
			curDPs.append(Datapoint(line, features))
	if len(curDPs) > 0:
		curBF._datapoints = curDPs
		BFs.append(curBF)

	if verbose is True:
		print("Found " + str(len(BFs)) + " benchmark functions")
		for BF in BFs:
			print("\t" + str(BF) + ": " + str(len(BF._datapoints)) + " datapoints")

	infile.close()
	return BFs

###############################################################################
# Model training
###############################################################################

#def writeTrainingData(benchFuncs, leaveOut):
#	global dataDir
#
#	fname = dataDir + "/lo_" + leaveOut.name() + ".csv"
#	if os.path.exists(fname):
#		return
#	else:
#		outfile = open(fname, "w")
#		for benchFunc in benchFuncs:
#			if benchFunc != leaveOut:
#				outfile.write(benchFunc.data() + "\n")
#		outfile.close()

def writeTestingData(benchFunc, featuresToKeep = None, subDir = ""):
	global dataDir

	fname = dataDir + "/" + subDir + "/" + benchFunc.name() + ".csv"
	if os.path.exists(fname):
		return
	else:
		outfile = open(fname, "w")
		outfile.write(benchFunc.data(featuresToKeep) + "\n")
		outfile.close()

def writeData(benchFuncs, featuresToKeep = None, subDir = ""):
	for benchFunc in benchFuncs:
	#	writeTrainingData(benchFuncs, benchFunc)
		writeTestingData(benchFunc, featuresToKeep, subDir)

###############################################################################
# Model training & testing
###############################################################################

def testNN(rate, momentum, pcaDim, hiddenDim, benchFuncs, otherArgs = None,
		subDir = "", extraConfig = "n/a"):
	global trainBin
	global dataDir
	global verbose
	global saveNNs
	global numArches

	# Build command-line
	args = [trainBin]
	for benchFunc in benchFuncs:
		args.append("--data")
		args.append(dataDir + "/" \
			+ subDir + "/" \
			+ benchFunc.name() + ".csv")

	args.extend(["--rate", str(rate), \
		"--momentum", str(momentum), \
		"--layer", str(hiddenDim), \
		"--iter", str(iterations), \
		"--arches", str(numArches)])

	if saveNNs is True:
		args.extend(["--save-nn", "--save-trans"])

	if str(pcaDim) == "no-pca": # In case we want to disable PCA
		args.append("--no-pca")
	else:
		args.extend(["--pca", str(pcaDim)])

	if otherArgs != None:
		args.extend(otherArgs)

	config = {
		"model" : "neural network",
		"rate" : rate,
		"momentum" : momentum,
		"pcaDim" : pcaDim,
		"hiddenDim" : hiddenDim,
		"extraConfig" : extraConfig
	}

	if verbose is True:
		print(args)

	print(" +++ Testing " + str(config))
	test(args, config)

def testDTree(benchFuncs, maxDepth, minSamplesPerCat, maxCategories):
	global trainBin
	global dataDir
	global verbose
	global numArches

	args = [trainBin]
	for benchFunc in benchFuncs:
		args.append("--data")
		args.append(dataDir + "/" + benchFunc.name() + ".csv")

	args.extend(["--no-cut-bad", \
		"--no-cut-prof", \
		"--no-cut-empty", \
		"--no-pca", \
		"--dtree", \
		"--depth", str(maxDepth), \
		"--min-samples", str(minSamplesPerCat), \
		"--max-cat", str(maxCategories), \
		"--arches", str(numArches)])

	config = {
		"model" : "decision tree",
		"maxDepth" : str(maxDepth),
		"minSamplesPerCat" : str(minSamplesPerCat),
		"maxCategories" : str(maxCategories)
	}

	if verbose is True:
		print(args)

	print(" +++ Testing " + str(config))
	test(args, config)

def test(args, config):
	global verbose
	global resultsQueue

	process = subprocess.Popen(args, shell=False, \
		stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = process.communicate()
	output = out.decode("utf-8")
	if verbose is True:
		print(output)
		print(err.decode("utf-8"))
	if process.returncode != 0:
		error("training failed")

	evaluateSection = False
	benchResults = ""
	for line in output.split("\n"):
		if "EVALUATE PREDICTOR" in line:
			evaluateSection = True
		elif evaluateSection is True and \
				"WARNING" not in line and \
				"BENCHMARK" not in line and \
				line != "":
			benchResults += line + "\n"
	resultsQueue.put(PredictorResults(config, benchResults))

###############################################################################
# Model configuration exploration
###############################################################################

def explore(benchFuncs):
	global concurrency
	global resultsQueue

	global testNNs
	global rates
	global momentums
	global pcaDims
	global hiddenDims

	global testDTrees
	global maxDepth
	global minSamplesPerCat
	global maxCategories

	writeData(benchFuncs)

	# Submit work to the thread pool to test neural networks
	if testNNs is True:
		header("Training neural networks")
		with concurrent.futures.ThreadPoolExecutor(max_workers=concurrency) as executor:
			for rate in rates:
				for momentum in momentums:
					for pcaDim in pcaDims:
						for hiddenDim in hiddenDims:
							executor.submit(testNN, rate, momentum, pcaDim, hiddenDim, benchFuncs)

	# Test decision trees
	# TODO allow list of each
	if testDTrees is True:
		header("Training decision trees")
		testDTree(benchFuncs, maxDepth, minSamplesPerCat, maxCategories)

	# Save results
	outfile = open("training_output.txt", "w")
	while not resultsQueue.empty():
		outfile.write(str(resultsQueue.get()) + "\n")
	outfile.close()

###############################################################################
# Greedy feature analysis
#
# Adapted from: "An Automatic Input-Sensitive Approach for Heterogeneous Task
#                Partitioning" by Kofler et. al
###############################################################################

def greedyFeatureAnalysis(benchFuncs):
	def testFeature(benchFuncs, greedilySelected, feature):
		global dataDir
		global rates
		global momentums
		global pcaDims
		global hiddenDims

		# TODO For now, limit analysis to a single NN configuration
		assert(len(rates) == 1 and
					 len(momentums) == 1 and
					 len(pcaDims) == 1 and
					 len(hiddenDims) == 1)

		# 1. Generate feature files w/ greedily selected & new feature
		f = copy.deepcopy(greedilySelected)
		f.append(feature)
		subDir = str(len(greedilySelected)) + "_" + str(feature)
		os.makedirs(dataDir + "/" + subDir)
		writeData(benchFuncs, f, subDir)

		# 2. Set PCA dimensions
		if pcaDims[0] == "no-pca":
			curPCA = "no-pca"
		else:
			if len(f) > pcaDims[0]:
				curPCA = pcaDims[0]  # Do PCA if # of greedy features >= PCA dims
			else:
				curPCA = "no-pca" # Else, no PCA

		testNN(rates[0], momentums[0], curPCA, hiddenDims[0], benchFuncs, \
			["--no-cut-bad", "--no-cut-prof", "--no-cut-empty"], subDir, feature)	

	global verbose
	global concurrency
	global staticFeatures
	global resultsQueue
	global numArches
	global maxSelected
	global selectionMetric

	S = [ i for i in range(len(benchFuncs[0].dp(0))-numArches) ] # All features
	F = [2, 4, 6, 10, 13, 14, 18, 19, 20] # Greedily selected features, start with "bad" features
	progression = [] # Testing progression
	improved = True # Prediction improved

	header("Performing greedy feature selection analysis")

	while improved and len(S) > 0 and len(F) < maxSelected:
		improved = False
		bestFeature = -1 # Best new feature
		bestCP = 0.0 # Current best percent correctly predicted
		bestRes = None
		curProgression = []

		with concurrent.futures.ThreadPoolExecutor(max_workers=concurrency) as executor:
			for s in S:
				if s == 2 or \
						s == 4 or \
						s == 6 or \
						s == 10 or \
						s == 13 or \
						s == 14 or \
						s == 18 or \
						s == 19 or \
						s == 20:
					continue
				else:
					executor.submit(testFeature, benchFuncs, F, s)

		while not resultsQueue.empty():
			curRes = resultsQueue.get()
			if selectionMetric == "speedup":
				curCP = float(curRes.speedup())
			else:
				curCP = float(curRes.correctlyPredicted())
			s = curRes.config("extraConfig")
			curProgression.append((staticFeatures[s], curCP))

			if curCP > bestCP:
				bestCP = curCP
				bestFeature = s
				bestRes = curRes
				improved = True

		if improved:
			S.remove(bestFeature)
			F.append(bestFeature)
			print(" +++ " + staticFeatures[bestFeature] + " (" + str(bestCP) + "%)")
			if verbose:
				print(bestRes)
		progression.append(curProgression)

	for prog in progression:
		print(prog)
	return progression # if the user wants it

###############################################################################
# Classes
###############################################################################

class Datapoint:
	def __init__(self, dpLine, features):
		self._features = {}
		self._ordered = [] #TODO change so there is only one copy
		split = dpLine.strip().split(",")
		for i in range(len(split)):
			self._features[features[i]] = float(split[i])
			self._ordered.append(float(split[i]))

	def __str__(self):
		data = ""
		for val in self._ordered:
			data += str(val) + ","
		return data[:len(data)-1]

	def data(self, featuresToKeep):
		global numArches

		# Add selected features
		data = ""
		for i in featuresToKeep:
			data += str(self._ordered[i]) + ","

		# Add labels
		for i in range(numArches):
			data += str(self._ordered[len(self._ordered)-numArches+i]) + ","
		return data[:len(data)-1]

class BenchFunc:
	def __init__(self, benchLine):
		split = benchLine.strip().split()
		self._bench = split[1]
		self._funcs = eval(benchLine[benchLine.find("["):benchLine.find("]")+1])

	def __str__(self):
		return self._bench + " - " + str(self._funcs)

	def name(self):
		return self._bench + "_" + self._funcs[0][:self._funcs[0].find('.')]

	def data(self, featuresToKeep = None):
		data = ""
		for dp in self._datapoints:
			if featuresToKeep is None:
				data += str(dp) + "\n"
			else:
				data += dp.data(featuresToKeep) + "\n"
		return data[:len(data)-1]

	# Access a datapoint's features
	def dp(self, datapoint):
		return self._datapoints[datapoint]._ordered

	# Access a single feature of a single datapoint
	def f(self, datapoint, feature):
		return self._datapoints[datapoint]._ordered[feature]

class PredictorResults:
	def __init__(self, config, results):
		self._config = config
		self._data = results

	def __str__(self):
		retval = "Results for " + self._config["model"] + ", "
		for key in self._config.keys():
			if key != "model":
				retval += key + " - " + str(self._config[key]) + ", "
		return retval[:len(retval)-2] + "\n" + self._data

	def correctlyPredicted(self):
		for line in self._data.split("\n"):
			if "Correctly classified" in line:
				return float(line.split()[4][1:-2])

	def speedup(self):
		for line in self._data.split("\n"):
			if "Speed-up" in line:
				return float(line.split()[2])

	def config(self, key):
		return self._config[key]

###############################################################################
# Driver
###############################################################################

parseArgs(sys.argv)
setup()
benchFuncs = parseFeatures(featureFile)
if greedyFeatureSelection:
	greedyFeatureAnalysis(benchFuncs)
elif testNNs or testDTrees:
	explore(benchFuncs)
cleanup()

