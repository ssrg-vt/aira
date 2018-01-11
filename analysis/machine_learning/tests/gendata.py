#!/usr/bin/env python3

import sys, os

def header_name(j):
  if j > 25:
    return "%s%s" % (header_name(int(j/26)-1), header_name(j%26))
  else:
    return str(chr(97+j))

def data_zeros(stride, i, j):
  return str(0)

def data_linear(stride, i, j):
  return str(i * stride + j + 1)

def data_123(stride, i, j):
  return str(((i * stride + j) % 3) + 1)

def data(generator, stride, i, j):
  gen = None
  if generator == "zeros":
    gen = data_zeros
  elif generator == "linear":
    gen = data_linear
  elif generator == "123":
    gen = data_123
  assert gen != None
  return gen(stride, i, j)

def usage(msg):
  print("ERROR: %s" % msg)
  print()
  print("Usage: ./gendata name_XxY_type.csv")
  print("\tname\tWhatever you like (no underscores).")
  print("\tX\tThe number of rows of data.")
  print("\tY\tThe number of columns of data.")
  print("\ttype\tGenerator type: zeros | linear | 123")
  print("\t\tzeros: All zeros.")
  print("\t\tlinear: Linearly increasing from 1.")
  print("\t\t123: Round-robin 1, 2, 3, 1, 2, 3 etc.")
  print()
  print("Will create a CSV file with contents fitting its file name.")
  sys.exit(1)

if __name__ == "__main__":
  if len(sys.argv) != 2:
    usage("Incorrect number of arguments.")
  arg = sys.argv[1].split(os.path.sep)[-1] # Just the filename of a path.

  if arg[-4:] != ".csv":
    usage("Output file must be of type csv.")

  args = arg[:-4].split("_")
  if len(args) != 3:
    usage("Invalid output filename.")

  # The data-set's name, we do nothing with this.
  name = args[0]

  # The data-set's size.
  sizes = args[1].split("x")
  if len(sizes) != 2:
    usage("Must provide a row (X) and column (Y) size.")
  try:
    rows = int(sizes[0])
    cols = int(sizes[1])
  except ValueError:
    usage("Row and column sizes must be integer numbers.")
  if rows <= 0 or cols <= 0:
    usage("Both row and column sizes must be greater than zero.")

  generator = args[2]
  if generator != "zeros" and generator != "linear" and generator != "123":
    usage("Unrecognised generator type (%s)." % generator)
  
  f = open(sys.argv[1], "wt")

  f.write(header_name(0))
  for j in range(1,cols):
    f.write(", %s" % header_name(j))
  f.write("\n")

  for i in range(0,rows):
    f.write("%s" % data(generator, cols, i, 0))
    for j in range(1,cols):
      f.write(", %s" % data(generator, cols, i, j))
    f.write("\n")
