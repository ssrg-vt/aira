/*****************************************************************************/
/* IRFeature class                                                           */
/*                                                                           */
/* This class defines a feature to be collected by the feature extractor at  */
/* the IR level.                                                             */
/*                                                                           */
/* It declares the virtual "collect" and "clear" functions, which collect    */
/* the feature from LLVM IR, and reset the feature for future functions,     */
/* respectively.                                                             */
/*                                                                           */
/* Additionally, this class defines a virtual "print" function which child   */
/* classes use to print human-readable output for each feature.              */
/*****************************************************************************/

#ifndef _IR_FEATURE_H
#define _IR_FEATURE_H

class IRFeature {
public:
	virtual void clear() = 0;
	virtual void collect(llvm::Instruction& instr, double bb_count) = 0;
	virtual void print(std::ostream& stream) = 0;
	//virtual ~IRFeature() = 0;

	static bool isVectorType(llvm::Value* val);

protected:
	static std::string name;
	static std::string desc;
};

#endif /* _IR_FEATURE_H */
