/* Hard-coded information for frankenstein */

#include "aira_definitions.h"
#include "server/system.h"

///////////////////////////////////////////////////////////////////////////////
// Predictor output slot information
///////////////////////////////////////////////////////////////////////////////

/* Number of predictor output slots */
const size_t prediction_slots = 6;

/* Default CPU device in the system (used to convert statistics into ratios) */
const size_t default_cpu = 0;

/* Device types of predictor output slots */
const cl_device_type device_types[] = {
	CL_DEVICE_TYPE_CPU, // Xeon E5-1650 v2 12C
	CL_DEVICE_TYPE_CPU, // '' 6C
	CL_DEVICE_TYPE_CPU, // '' 4C
	CL_DEVICE_TYPE_CPU, // '' 3C
	CL_DEVICE_TYPE_CPU, // '' 2C
	CL_DEVICE_TYPE_GPU  // Titan
};

/* Map prediction output slots to struct resource_allocation */
const struct resource_alloc system_devices[] = {
	{ // Xeon E5-1650 v2 12C
		.platform = 0,
		.device = 0,
		.compute_units = 12
	},

	{ // Xeon E5-1650 v2 6C
		.platform = 0,
		.device = 0,
		.compute_units = 6
	},

	{ // Xeon E5-1650 v2 4C
		.platform = 0,
		.device = 0,
		.compute_units = 4
	},

	{ // Xeon E5-1650 v2 3C
		.platform = 0,
		.device = 0,
		.compute_units = 3
	},

	{ // Xeon E5-1650 v2 2C
		.platform = 0,
		.device = 0,
		.compute_units = 2
	},

	{ // Titan
		.platform = 1,
		.device = 0,
		.compute_units = 14
	}
};

///////////////////////////////////////////////////////////////////////////////
// Benchmark statistics
///////////////////////////////////////////////////////////////////////////////

/* NPB runtime statistics */
const float rt_mean = 0.0f;
const float rt_max = 0.0f;
const float rt_min = 0.0f;

/* NPB benchmark runtimes on each architecture -- values are in seconds. */
const float runtime [][40] = {
	{ // Xeon E5-1650 v2 12C
		0.055f, 0.418f,  9.176f,  37.230f, 151.854f, // BT (S, W, A, B, C)
		0.076f, 0.123f,  0.540f,  17.309f,  46.575f, // CG
		0.069f, 0.099f,  0.597f,   2.205f,   8.680f, // EP
		0.046f, 0.107f,  1.694f,  18.725f,  66.996f, // FT
		0.042f, 0.052f,  0.136f,   0.550f,   2.430f, // IS
    0.071f, 1.695f,  7.408f,  27.309f, 103.100f, // LU
		0.019f, 0.130f,  0.877f,   2.796f,  26.666f, // MG
		0.077f, 1.042f, 13.524f,  56.802f, 231.729f  // SP
	},

	// TODO populate!
	{ // Xeon E5-1650 v2 6C
		0.071f, 0.604f, 12.937f,  56.521f, 224.381f, // BT (S, W, A, B, C)
		0.071f, 0.160f,  0.619f,  28.258f,  79.423f, // CG
		0.108f, 0.161f,  1.014f,   4.099f,  15.668f, // EP
		0.052f, 0.112f,  1.652f,  17.647f,  71.167f, // FT
		0.063f, 0.084f,  0.206f,   0.795f,   3.077f, // IS
    0.073f, 1.954f,  9.524f,  34.992f, 141.945f, // LU
		0.016f, 0.137f,  0.881f,   2.829f,  25.762f, // MG
		0.081f, 1.496f, 14.775f,  61.060f, 249.695f  // SP
	},

	// TODO populate!
	{ // Xeon E5-1650 v2 4C
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // BT (S, W, A, B, C)
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // CG
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // EP
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // FT
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // IS
    0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // LU
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // MG
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f  // SP
	},

	// TODO populate!
	{ // Xeon E5-1650 v2 3C
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // BT (S, W, A, B, C)
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // CG
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // EP
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // FT
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // IS
    0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // LU
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // MG
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f  // SP
	},

	// TODO populate!
	{ // Xeon E5-1650 v2 2C
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // BT (S, W, A, B, C)
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // CG
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // EP
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // FT
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // IS
    0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // LU
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f, // MG
		0.000f, 0.000f,  0.000f,   0.000f,   0.000f  // SP
	},

	{ // Titan
		0.230f, 2.020f, 24.307f,  80.360f, 356.812f, // BT (S, W, A, B, C)
		0.154f, 0.399f,  1.163f,  15.342f,  42.921f, // CG
		0.179f, 0.178f,  0.190f,   0.561f,   2.049f, // EP
		0.033f, 0.090f,  2.120f,  55.193f, 223.557f, // FT
		0.002f, 0.005f,  0.020f,   0.300f,   1.600f, // IS
		0.081f, 1.835f,  3.947f,  12.518f,  41.870f, // LU
		0.020f, 0.120f,  0.555f,   0.924f,   6.776f, // MG
		0.160f, 2.170f,  7.689f,  34.305f, 169.086f  // SP
	}
};

/* NPB energy consumption statistics */
const float energy_mean = 0.0f;
const float energy_max = 0.0f;
const float energy_min = 0.0f;

/* NPB benchmark energy consumption on each architecture -- values are in
 * joules. */
const float energy [][40] = {
	{ // Xeon E5-1650 v2 12C
		13.616f,  61.980f, 1210.679f,  5304.680f, 23232.412f, // BT (S, W, A, B, C)
		14.581f,  23.400f,   72.104f,  2099.540f,  5964.362f, // CG
		13.726f,  18.102f,   78.471f,   290.230f,  1151.771f, // EP
		12.634f,  19.545f,  201.036f,  2212.383f,    -1.000f, // FT
		10.670f,  13.071f,   25.936f,    70.781f,   278.570f, // IS
		16.763f, 231.007f,  989.132f,  3694.607f, 14900.943f, // LU
		 9.680f,  23.262f,  109.577f,   343.425f,  3351.597f, // MG
		17.332f, 139.610f, 1575.941f,  7145.612f, 30472.535f  // SP
	},

	// TODO populate!
	{ // Xeon E5-1650 v2 6C
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // BT (S, W, A, B, C)
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // CG
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // EP
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // FT
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // IS
     0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // LU
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // MG
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f  // SP
	},

	// TODO populate!
	{ // Xeon E5-1650 v2 4C
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // BT (S, W, A, B, C)
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // CG
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // EP
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // FT
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // IS
     0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // LU
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // MG
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f  // SP
	},

	// TODO populate!
	{ // Xeon E5-1650 v2 3C
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // BT (S, W, A, B, C)
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // CG
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // EP
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // FT
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // IS
     0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // LU
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // MG
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f  // SP
	},

	// TODO populate!
	{ // Xeon E5-1650 v2 2C
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // BT (S, W, A, B, C)
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // CG
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // EP
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // FT
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // IS
     0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // LU
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f, // MG
		 0.000f,   0.000f,    0.000f,     0.000f,     0.000f  // SP
	},

	{ // Titan
		33.945f, 268.570f, 4234.350f, 15064.131f, 67815.327f, // BT (S, W, A, B, C)
		32.833f,  60.867f,  165.863f,  2466.344f,  7561.782f, // CG
		37.197f,  37.978f,   70.123f,   168.682f,   613.904f, // EP
		21.646f,  43.140f,  522.892f,  6391.793f,     0.000f, // FT
		16.343f,  16.213f,   17.467f,    73.812f,   295.042f, // IS
		23.495f, 205.948f,  561.599f,  2041.041f,  7681.483f, // LU
		17.756f,  37.486f,   92.671f,   172.451f,  1184.692f, // MG
		36.383f, 330.423f, 1402.668f,  6592.085f, 34275.222f // SP
	}
};

