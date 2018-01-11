/*
 * architecture_finder_common.h
 *
 *  Created on: Apr 5, 2013
 *      Author: rlyerly
 */

#ifndef ARCHITECTURE_FINDER_COMMON_H_
#define ARCHITECTURE_FINDER_COMMON_H_

/*
 * Useful definitions
 */
#define TOOL "architecture_finder"

/*
 * Used to delineate recursive calls from each other.
 */
#undef DEBUG
#define DEBUG(tool, msg) {cerr << tool << " (Info): "; \
for(int i = 0; i < numTabs; i++) cerr << "\t"; \
cerr << msg << endl;}
extern int numTabs;

#endif /* ARCHITECTURE_FINDER_COMMON_H_ */
