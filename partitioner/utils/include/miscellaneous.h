/*
 * utils.h
 *
 *  Created on: Jul 22, 2013
 *      Author: rlyerly
 */

#ifndef UTILS_H_
#define UTILS_H_

namespace Misc {

void getType(SgType** origType, SgType** baseType, int* numDimensions);
bool classContainsPointers(SgClassType* type);

} /* namespace Utils */
#endif /* UTILS_H_ */
