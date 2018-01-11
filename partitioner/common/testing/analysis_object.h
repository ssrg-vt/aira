/*
 * analysis_object.h
 *
 * Abstract base class which all analysis objects inherit.  Defines the common
 * analyze/annotate interface required by all analysis objects.
 *
 * Created on: Apr 17, 2013
 * Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef ANALYSIS_OBJECT_H_
#define ANALYSIS_OBJECT_H_

#include "common.h"

class AnalysisObject {
public:
	AnalysisObject(SgFunctionDeclaration*, SgGraphNode*) = 0;
	SgGraphNode* getGraphNode() { return node; }
	void setFunctionsCalled(set<AnalysisObject*> p_functionsCalled)
		{ functionsCalled = p_functionsCalled; }
	virtual void analyze() = 0;
	virtual void annotate() = 0;

protected:
	SgGraphNode* node;
	enum analysisStatus status;
	set<AnalysisObject*> functionsCalled;
}

#endif /* ANALYSIS_OBJECT_H_ */
