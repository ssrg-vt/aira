#include <string>
#include <vector>
#include <gsl/gsl_matrix.h>

#include "persistent.hh"

#ifndef _TRANSFORM_H
#define _TRANSFORM_H

class Transform
{
public:
	enum type {
		ZEROMEAN = 0,
		RANGESCALE,
		STDDEVSCALE
	};

	Transform(enum type transType, size_t numFeats)
		: ttype(transType), numFeatures(numFeats) {}

	enum type getType() const { return ttype; }
	size_t getNumFeatures() const { return numFeatures; }

	virtual void calculate(const gsl_matrix* features) = 0;
	virtual void apply(std::vector<double>& features) = 0;
	virtual void apply(gsl_matrix* features) = 0;
	virtual std::string getData() const = 0;
	virtual void parseData(std::string& data) = 0;

protected:
	enum type ttype;
	size_t numFeatures;
};

class ZeroMean : public Transform
{
public:
	ZeroMean(size_t numFeatures)
		: Transform(ZEROMEAN, numFeatures), shift(numFeatures) {}

	virtual void calculate(const gsl_matrix* features);
	virtual void apply(std::vector<double>& features);
	virtual void apply(gsl_matrix* features);
	virtual std::string getData() const;
	virtual void parseData(std::string& data);

private:
	std::vector<double> shift;
};

class RangeScale : public Transform
{
public:
	RangeScale(size_t numFeatures)
		: Transform(RANGESCALE, numFeatures), max(numFeatures), min(numFeatures) {}

	virtual void calculate(const gsl_matrix* features);
	virtual void apply(std::vector<double>& features);
	virtual void apply(gsl_matrix* features);
	virtual std::string getData() const;
	virtual void parseData(std::string& data);

private:
	std::vector<double> max;
	std::vector<double> min;
};

class StdDevScale : public Transform
{
public:
	StdDevScale(size_t numFeatures)
		: Transform(STDDEVSCALE, numFeatures), mean(numFeatures),
			stddev(numFeatures)	{}

	virtual void calculate(const gsl_matrix* features);
	virtual void apply(std::vector<double>& features);
	virtual void apply(gsl_matrix* features);
	virtual std::string getData() const;
	virtual void parseData(std::string& data);

private:
	std::vector<double> mean;
	std::vector<double> stddev;
};

class TransformManager : public Persistent
{
public:
	void addTransform(Transform* transform);
	void applyTransforms(gsl_matrix* features);
	virtual int save(std::string& filename);
	virtual int load(std::string& filename);

private:
	std::vector<Transform*> transforms;
};

#endif /* _TRANSFORM_H */
