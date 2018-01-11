#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <algorithm>
#include <cassert>

#include "data.hh"

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

Datafile::Datafile(size_t numArches)
	: rows(0), cols(0), features(0), labels(numArches), x(NULL), y(numArches) {}

Datafile::~Datafile()
{
	gsl_matrix_free(x);
	for(gsl_vector* vec : y)
		gsl_vector_free(vec);
}

void Datafile::labelsToRatios()
{
	for(size_t i = 0; i < rows; i++)
	{
		double baseline = gsl_vector_get(y[0], i);
		for(size_t j = 0; j < labels; j++)
			gsl_vector_set(y[j], i, baseline / gsl_vector_get(y[j], i));
	}
}

int Datafile::save(std::string& filename)
{
	// TODO
	assert(false && "Not yet implemented!");
	return 0;
}

int Datafile::load(std::string& filename)
{
	this->filename = filename;
	std::ifstream input;
	input.open(filename.c_str());
	if(!input.is_open())
		return -1;

	rows = numLines(input);
	cols = numCols(input);
	features = cols - labels;
	x = gsl_matrix_alloc(rows, features);
	for(size_t i = 0; i < labels; i++)
		y[i] = gsl_vector_alloc(rows);
	loadData(input);

	input.close();
	return 0; // No problem mahn
}

///////////////////////////////////////////////////////////////////////////////
// Private API
///////////////////////////////////////////////////////////////////////////////

size_t Datafile::numLines(std::ifstream& file)
{
	file.unsetf(std::ios_base::skipws); // Need to undo?
	size_t lines = std::count(std::istream_iterator<char>(file),
														std::istream_iterator<char>(), '\n');
	file.clear();
	file.seekg(0);
	return lines;
}

size_t Datafile::numCols(std::ifstream& file)
{
	std::string line;
	std::getline(file, line);
	file.clear();
	file.seekg(0);
	return std::count(line.begin(), line.end(), ',') + 1;
}

void Datafile::loadData(std::ifstream& file)
{
	std::string line, tok;
	for(size_t i = 0; i < rows; i++)
	{
		std::getline(file, line);
		std::stringstream ss(line);

		// Extract features
		for(size_t j = 0; j < features; j++)
		{
			std::getline(ss, tok, ',');
			gsl_matrix_set(x, i, j, std::stod(tok));
		}

		// Extract labels
		for(size_t j = 0; j < labels; j++)
		{
			std::getline(ss, tok, ',');
			gsl_vector_set(y[j], i, std::stod(tok));
		}
	}
}

