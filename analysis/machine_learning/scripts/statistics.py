#!/usr/bin/python3

import sys,os,math,subprocess

###############################################################################
# Config
###############################################################################

infile = "n/a"
printNum = False
graphHisto = False
graphScatter = False
histoData = "distributions.dat"
histoFile = "distributions.pdf"
scatterData = "scatterplot.dat"
scatterFile = "scatterplot.pdf"
columns = []

###############################################################################
# Helpers
###############################################################################

def printHelp():
	print("statistics.py - print basic statistics generated from a .csv file\n")
	print("Usage: statistics.py <CSV file> [ OPTIONS ]\n")
	print("Options:")
	print("\t-h              : print this help")
	print("\t-n              : print number of columns & quit")
	print("\t-c <comma list> : print statistics for the specified columns (default is all)")
	print("\t-d <graph name> : generate histogram of column distributions (default is " + histoFile + ", must end in .pdf)")
	print("\t-s <graph name> : generate scatterplot of column values (default is " + scatterFile + ", must end in .pdf)")

def parseArgs(args):
	global infile
	global columns
	global printNum
	global graphHisto
	global graphScatter
	global histoFile
	global scatterFile

	if len(args) < 2:
		print("Please specify an input CSV file!\n")
		printHelp()
		sys.exit(1)

	skip = False
	for i in range(len(args)):
		if skip is True:
			skip = False
		elif i == 1:
			if args[i] == "-h" or args[i] == "--help":
				printHelp()
				sys.exit(0)
			else:
				infile = args[i]
		elif args[i] == "-h":
			printHelp()
			sys.exit(0)
		elif args[i] == "-c":
			columns = [ int(x) for x in args[i+1].split(",") ]
			skip = True
		elif args[i] == "-n":
			printNum = True
		elif args[i] == "-d":
			graphHisto = True
			if (i+1) < len(args) and ".pdf" in args[i+1]:
				histoFile = args[i+1]
				skip = True
		elif args[i] == "-s":
			graphScatter = True
			if (i+1) < len(args) and ".pdf" in args[i+1]:
				scatterFile = args[i+1]
				skip = True

###############################################################################
# Parsing
###############################################################################

def parseFile():
	global infile
	global columns
	global printNum

	# Get number of columns & initialize stats objects
	f = open(infile, "r")
	line = f.readline()
	numCols = len(line.split(","))
	stats = [ ColumnStats(x) for x in range(numCols) ]
	f.seek(0)

	if printNum is not True:
		if len(columns) == 0:
			columns = [ x for x in range(numCols) ]

		for line in f:
			vals = line.split(",")
			for i in columns:
				stats[i].add(vals[i])

	f.close()
	return stats

def printStats(stats):
	def getMaxWidth(data, column):
		return max([len(str(row[column])) for row in data])

	colWidths = []
	for i in range(len(stats[0])):
		colWidths.append(getMaxWidth(stats, i))

	for row in stats:
		print(str(row[0]).ljust(colWidths[0]+1), end="")
		for i in range(1, len(row)):
			print(str(row[i]).rjust(colWidths[i]+2), end="")
		print()

###############################################################################
# Classes
###############################################################################

class ColumnStats:
	def __init__(self, column):
		self._column = column
		self._vals = []
		self._max = sys.float_info.min
		self._min = sys.float_info.max
		self._avg = 0.0
		self._stddev = 0.0

	def add(self, val):
		self._vals.append(float(val))

	def calcStats(self):
		self._max = sys.float_info.min
		self._min = sys.float_info.max
		self._avg = 0.0
		self._stddev = 0.0

		# Calculate average, max & min
		for val in self._vals:
			self._avg += val
			if val > self._max:
				self._max = val
			if val < self._min:
				self._min = val
		self._avg /= len(self._vals)

		# Calculate stddev
		for val in self._vals:
			self._stddev += (val - self._avg)**2
		self._stddev /= len(self._vals)
		self._stddev = math.sqrt(self._stddev)

		return [self._column, self._max, self._min, self._avg, self._stddev, \
			self._stddev/self._avg]

	def toDistribution(self):
		buckets = [ 0 for x in range(10) ]
		_range = self._max - self._min
		for val in self._vals:
			percent = (val - self._min)/_range
			buckets[int(percent * 10)-1] += 1
		for i in range(len(buckets)):
			buckets[i] = float(buckets[i]/len(self._vals)) * 100
		return buckets

	def __str__(self):
		return str(self._column) + ": " + str(self._max) + ", " + str(self._min) \
			+ ", " + str(self._avg) + ", " + str(self._stddev)

###############################################################################
# Driver
###############################################################################

parseArgs(sys.argv)
stats = parseFile()

if printNum is True:
	# Print number of columns
	print("Number of columns: " + str(len(stats)))
elif graphHisto is True:
	# Get distributions from specified columns
	colBuckets = []
	for col in columns:
		stats[col].calcStats()
		colBuckets.append(stats[col].toDistribution())

	# Write distributions to file
	data = ""
	for col in columns:
		data += "Column " + str(col) + ","
	data = data[:len(data)-1] + "\n"
	for i in range(len(colBuckets[0])):
		for bucket in colBuckets:
			data += str(bucket[i]) + ","
		data = data[:len(data)-1] + "\n"
	gf = open(histoData, "w")	
	gf.write(data)
	gf.close()

	# Graph & delete data file
	subprocess.call(["gnuplot", \
		"-e", "num_cols=" + str(len(columns)), \
		"-e", "outfile=\"" + histoFile + "\"", \
		"plot_distributions.gp"])
	os.remove(histoData)
elif graphScatter is True:
	# Get values from the specified columns
	data = ""
	for col in columns:
		data += "Column " + str(col) + ","
	data = data[:len(data)-1] + "\n"
	for i in range(len(stats[columns[0]]._vals)):
		for col in columns:
			data += str(stats[col]._vals[i]) + ","
		data = data[:len(data)-1] + "\n"

	gf = open(scatterData, "w")
	gf.write(data)
	gf.close()

	# Graph & delete data file
	subprocess.call(["gnuplot", \
		"-e", "num_cols=" + str(len(columns)), \
		"-e", "outfile=\"" + scatterFile + "\"", \
		"plot_scatterplot.gp"])
	os.remove(scatterData)
else:
	# Print simple statistics
	selectedStats = [["Column", "Max", "Min", "Average", "Std. Deviation", "Std. Dev. / Avg"]]
	for col in columns:
		selectedStats.append(stats[col].calcStats())
	printStats(selectedStats)

