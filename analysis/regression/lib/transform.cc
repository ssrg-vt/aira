#include <iostream>
#include <sstream>
#include <fstream>
#include <limits>
#include <cmath>
#include <cassert>

#include "transform.hh"

///////////////////////////////////////////////////////////////////////////////
// ZeroMean
///////////////////////////////////////////////////////////////////////////////

void ZeroMean::calculate(const gsl_matrix* features)
{
	for(size_t j = 0; j < numFeatures; j++)
		shift[j] = 0.0;

	for(size_t i = 0; i < features->size1; i++)
		for(size_t j = 0; j < features->size2; j++)
			shift[j] += gsl_matrix_get(features, i, j);

	for(size_t j = 0; j < numFeatures; j++)
		shift[j] /= features->size1;
}

void ZeroMean::apply(std::vector<double>& features)
{
	assert(features.size() == numFeatures);
	for(size_t j = 0; j < features.size(); j++)
		features[j] -= shift[j];
}

void ZeroMean::apply(gsl_matrix* features)
{
	for(size_t i = 0; i < features->size1; i++)
		for(size_t j = 0; j < features->size2; j++)
			gsl_matrix_set(features, i, j, gsl_matrix_get(features, i, j) - shift[j]);
}

std::string ZeroMean::getData() const
{
	std::stringstream ss;
	std::string data;
	ss << shift[0];
	ss >> data;
	for(size_t i = 1; i < shift.size(); i++)
	{
		ss.clear();
		std::string val;
		ss << shift[i];
		ss >> val;
		data += "," + val;
	}
	return data;
}

void ZeroMean::parseData(std::string& data)
{
	std::stringstream ss;
	std::string tok;

	ss << data;
	for(size_t i = 0; i < numFeatures; i++)
	{
		std::getline(ss, tok, ',');
		shift[i] = std::stod(tok);
	}
}

///////////////////////////////////////////////////////////////////////////////
// RangeScale
///////////////////////////////////////////////////////////////////////////////

void RangeScale::calculate(const gsl_matrix* features)
{
	for(size_t j = 0; j < numFeatures; j++)
	{
		max[j] = std::numeric_limits<double>::min();
		min[j] = std::numeric_limits<double>::max();
	}

	for(size_t i = 0; i < features->size1; i++) {
		for(size_t j = 0; j < features->size2; j++) {
			if(gsl_matrix_get(features, i, j) > max[j])
				max[j] = gsl_matrix_get(features, i, j);
			else if(gsl_matrix_get(features, i, j) < min[j])
				min[j] = gsl_matrix_get(features, i, j);
		}
	}

	for(size_t j = 0; j < numFeatures; j++)
		if(max[j] == 0.0 && min[j] == 0.0)
			max[j] = 1.0;
}

void RangeScale::apply(std::vector<double>& features)
{
	assert(features.size() == numFeatures);
	for(size_t j = 0; j < numFeatures; j++)
		features[j] = (features[j] - min[j]) / (max[j] - min[j]);
}

void RangeScale::apply(gsl_matrix* features)
{
	for(size_t i = 0; i < features->size1; i++) {
		
		for(size_t j = 0; j < features->size2; j++) {
			double val = gsl_matrix_get(features, i, j);
			val = (val - min[j]) / (max[j] - min[j]);
			gsl_matrix_set(features, i, j, val);
		}
	}
}

std::string RangeScale::getData() const
{
	std::stringstream ss;
	std::string data, val;

	// 1st line: max
	ss << max[0];
	ss >> data;
	for(size_t i = 1; i < max.size(); i++)
	{
		ss.clear();
		ss << max[i];
		ss >> val;
		data += "," + val;
	}
	data += "\n";

	// 2nd line: min
	ss.clear();
	ss << min[0];
	ss >> val;
	data += val;
	for(size_t i = 1; i < min.size(); i++)
	{
		ss.clear();
		ss << min[i];
		ss >> val;
		data += "," + val;
	}

	return data;
}

void RangeScale::parseData(std::string& data)
{
	std::stringstream ss;
	std::string maxLine, minLine, tok;

	ss << data;
	std::getline(ss, maxLine);
	std::getline(ss, minLine);
	ss.clear();
	ss << maxLine;
	for(size_t i = 0; i < numFeatures; i++)
	{
		std::getline(ss, tok, ',');
		max[i] = std::stod(tok);
	}

	ss.clear();
	ss << minLine;
	for(size_t i = 0; i < numFeatures; i++)
	{
		std::getline(ss, tok, ',');
		min[i] = std::stod(tok);
	}
}

///////////////////////////////////////////////////////////////////////////////
// StdDevScale
///////////////////////////////////////////////////////////////////////////////

void StdDevScale::calculate(const gsl_matrix* features)
{
	for(size_t j = 0; j < numFeatures; j++)
	{
		mean[j] = 0;
		stddev[j] = 0;
	}

	// Calculate mean
	for(size_t i = 0; i < features->size1; i++)
		for(size_t j = 0; j < features->size2; j++)
			mean[j] += gsl_matrix_get(features, i, j);
	for(size_t j = 0; j < numFeatures; j++)
		mean[j] /= features->size1;

	// Calculate standard deviation
	for(size_t i = 0; i < features->size1; i++)
		for(size_t j = 0; j < features->size2; j++)
			stddev[j] += pow(gsl_matrix_get(features, i, j) - mean[j], 2);
	for(size_t j = 0; j < numFeatures; j++)
	{
		stddev[j] = sqrt(stddev[j] / features->size1);
		if(stddev[j] == 0.0)
			stddev[j] = 1.0;
	}
}

void StdDevScale::apply(std::vector<double>& features)
{
	assert(features.size() == numFeatures);
	for(size_t j = 0; j < numFeatures; j++)
		features[j] = (features[j] - mean[j]) / stddev[j];
}

void StdDevScale::apply(gsl_matrix* features)
{
	for(size_t i = 0; i < features->size1; i++) {
		for(size_t j = 0; j < features->size2; j++) {
			double val = gsl_matrix_get(features, i, j);
			val = (val - mean[j]) / stddev[j];
			gsl_matrix_set(features, i, j, val);
		}
	}
}

std::string StdDevScale::getData() const
{
	std::stringstream ss;
	std::string data, val;

	// 1st line: mean
	ss << mean[0];
	ss >> data;
	for(size_t i = 1; i < mean.size(); i++)
	{
		ss.clear();
		ss << mean[i];
		ss >> val;
		data += "," + val;
	}
	data += "\n";

	// 2nd line: stddev
	ss.clear();
	ss << stddev[0];
	ss >> val;
	data += val;
	for(size_t i = 1; i < stddev.size(); i++)
	{
		ss.clear();
		ss << stddev[i];
		ss >> val;
		data += "," + val;
	}

	return data;
}

void StdDevScale::parseData(std::string& data)
{
	std::stringstream ss;
	std::string meanLine, stddevLine, tok;

	ss << data;
	std::getline(ss, meanLine);
	std::getline(ss, stddevLine);
	ss.clear();
	ss << meanLine;
	for(size_t i = 0; i < numFeatures; i++)
	{
		std::getline(ss, tok, ',');
		mean[i] = std::stod(tok);
	}

	ss.clear();
	ss << stddevLine;
	for(size_t i = 0; i < numFeatures; i++)
	{
		std::getline(ss, tok, ',');
		stddev[i] = std::stod(tok);
	}
}

///////////////////////////////////////////////////////////////////////////////
// TransformManager
///////////////////////////////////////////////////////////////////////////////

void TransformManager::addTransform(Transform* transform)
{
	assert(transform);
	transforms.push_back(transform);
}

void TransformManager::applyTransforms(gsl_matrix* features)
{
	for(Transform* tf : transforms)
		tf->apply(features);
}

int TransformManager::save(std::string& filename)
{
	using boost::property_tree::ptree;
	using boost::property_tree::write_json;

	ptree pt;
	pt.put("num_trans", transforms.size());
	pt.put("num_features", transforms[0]->getNumFeatures());
	for(size_t i = 0; i < transforms.size(); i++)
	{
		std::stringstream ss;
		std::string base;
		ss << i;
		ss >> base;
		base += ".";

		std::string val;
		switch(transforms[i]->getType())
		{
		case Transform::ZEROMEAN:
			pt.put(base + "type", "zeromean");
			break;
		case Transform::RANGESCALE:
			pt.put(base + "type", "rangescale");
			break;
		case Transform::STDDEVSCALE:
			pt.put(base + "type", "stddevscale");
			break;
		default:
			pt.put(base + "type", "unknown");
			break;
		}
		pt.put(base + "data", transforms[i]->getData());
	}
	write_json(filename, pt);
	return 0;
}

int TransformManager::load(std::string& filename)
{
	using boost::property_tree::ptree;
	using boost::property_tree::read_json;

	ptree pt;
	Transform* trans;

	read_json(filename, pt);
	size_t numTrans = pt.get<size_t>("num_trans");
	size_t numFeatures = pt.get<size_t>("num_features");
	for(size_t i = 0; i < numTrans; i++)
	{
		std::stringstream ss;
		std::string base;
		ss << i;
		ss >> base;
		base += ".";

		std::string type = pt.get<std::string>(base + "type");
		std::string data = pt.get<std::string>(base + "data");
		if(type == "zeromean") trans = new ZeroMean(numFeatures);
		else if(type == "rangescale") trans = new RangeScale(numFeatures);
		else if(type == "stddevscale") trans = new StdDevScale(numFeatures);
		trans->parseData(data);
		transforms.push_back(trans);
	}
	return 0;
}

