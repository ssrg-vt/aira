/*
 * Enumeration/string array defining kernels being run.  This is mainly used
 * for testing purposes so that the client can easily notify the server of
 * which kernel is about to be run (in case the server wants to handle the
 * kernel in a special manner).  This would not be used in real systems.
 *
 * Author: Rob Lyerly
 * Date: 9/11/2015
 */

#ifndef _KERNELS_H
#define _KERNELS_H

#ifdef __cplusplus
extern "C" {
#endif

#define NPB_KERNELS \
	X(BT_S, "BT.S") \
	X(BT_W, "BT.W") \
	X(BT_A, "BT.A") \
	X(BT_B, "BT.B") \
	X(BT_C, "BT.C") \
	X(CG_S, "CG.S") \
	X(CG_W, "CG.W") \
	X(CG_A, "CG.A") \
	X(CG_B, "CG.B") \
	X(CG_C, "CG.C") \
	X(EP_S, "EP.S") \
	X(EP_W, "EP.W") \
	X(EP_A, "EP.A") \
	X(EP_B, "EP.B") \
	X(EP_C, "EP.C") \
	X(FT_S, "FT.S") \
	X(FT_W, "FT.W") \
	X(FT_A, "FT.A") \
	X(FT_B, "FT.B") \
	X(FT_C, "FT.C") \
	X(IS_S, "IS.S") \
	X(IS_W, "IS.W") \
	X(IS_A, "IS.A") \
	X(IS_B, "IS.B") \
	X(IS_C, "IS.C") \
	X(LU_S, "LU.S") \
	X(LU_W, "LU.W") \
	X(LU_A, "LU.A") \
	X(LU_B, "LU.B") \
	X(LU_C, "LU.C") \
	X(MG_S, "MG.S") \
	X(MG_W, "MG.W") \
	X(MG_A, "MG.A") \
	X(MG_B, "MG.B") \
	X(MG_C, "MG.C") \
	X(SP_S, "SP.S") \
	X(SP_W, "SP.W") \
	X(SP_A, "SP.A") \
	X(SP_B, "SP.B") \
	X(SP_C, "SP.C")

enum npb_kernels
{
#define X(a, b) a,
NPB_KERNELS
#undef X
};

extern const char* npb_kernel_names[];

#ifdef __cplusplus
}
#endif

#endif
