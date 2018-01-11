/*
 * arch_selector.h
 *
 *  Created on: Apr 24, 2013
 *      Author: rlyerly
 */

#ifndef ARCH_SELECTOR_H_
#define ARCH_SELECTOR_H_

class ArchSelector {
public:
	ArchSelector(PragmaParser& pp, PragmaParser& allowed);

	bool partitionOntoGpu();
	bool partitionOntoMpi();

private:
	bool useGpu;
	bool useMpi;
};

#endif /* ARCH_SELECTOR_H_ */
