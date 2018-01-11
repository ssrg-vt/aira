/*
 * IntelRAPLDevice.h
 *
 *  Created on: May 13, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SRC_DEVICE_INTELRAPLDEVICE_H_
#define SRC_DEVICE_INTELRAPLDEVICE_H_

#include "IntelDevice.h"

enum RAPLClass {
	CLIENT = 0,
	SERVER,
	NOT_YET_DETECTED
};

class IntelRAPLDevice: public IntelDevice {
public:
	IntelRAPLDevice();
	virtual ~IntelRAPLDevice();

	// Power/energy accounting
	virtual retval_t startEnergyAccounting();
	retval_t addEnergyConsumption(struct timespec& time);
	virtual retval_t resetDeviceExpenditure();

	// Getters
	virtual std::string power() const;
	unsigned numCPUs() const { return _cpus.size(); }
	int msrFD() const { return _msrFD; }
	double energyMultiplier() const { return _esu; }
	enum RAPLClass type() const { return _class; }
	bool pp1Supported() const { return _pp1Enabled; }
	bool dramSupported() const { return _dramEnabled; }

	// Setters
	void addCPU(int cpuNum) { _cpus.push_back(cpuNum); }
	void energyMultiplier(unsigned multiplier) { _esu = 1.0 / (double)(2 << (multiplier-1)); }

	// RAPL operations
	retval_t initializeRAPL();

private:
	enum RAPLClass _class;
	std::vector<int> _cpus;
	int _msrFD;
	double _esu; // Energy status units (i.e. multiplier), in Joules

	// Power & energy accounting
	// Note: _power = package power & _energy = package energy
	bool _pp1Enabled, _dramEnabled;
	double _pp0Power, _pp1Power, _dramPower;
	double _pp0Energy, _pp1Energy, _dramEnergy;
	unsigned _prevPkg, _prevPP0, _prevPP1, _prevDRAM;

	// Functions
	/* RAPL Device Actions */
	retval_t _measureRAPLEnergy(unsigned& pkg, unsigned& pp0,
								unsigned& pp1, unsigned& dram);
	void _updateRAPLAccounting(unsigned newReading, unsigned& oldReading,
							   double& energy, double& power,
							   struct timespec& time);
};

#endif /* SRC_DEVICE_INTELRAPLDEVICE_H_ */
