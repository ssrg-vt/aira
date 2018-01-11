###############################################################################
# Building
###############################################################################

# benchmarks
all: analysis opencl_runtime power_management load_balancer

analysis:
	$(MAKE) -C ./analysis/regression
	$(MAKE) -C ./analysis/machine_learning

opencl_runtime:
	$(MAKE) -C ./opencl_runtime

power_management:
	$(MAKE) -C ./power_management

load_balancer: analysis opencl_runtime
	$(MAKE) -C ./load_balancer

#benchmarks: load_balancer power_management
#	$(MAKE) -C ./benchmarks/SNU_NPB-1.0.3/NPB3.3-OCL suite dyn_adj=enable

###############################################################################
# Cleaning
###############################################################################

analysis_clean:
	$(MAKE) -C ./analysis/regression clean
	$(MAKE) -C ./analysis/machine_learning clean

opencl_runtime_clean:
	$(MAKE) -C ./opencl_runtime clean

power_management_clean:
	$(MAKE) -C ./power_management clean

load_balancer_clean:
	$(MAKE) -C ./load_balancer clean

benchmarks_clean:
	$(MAKE) -C ./benchmarks/SNU_NPB-1.0.3/NPB3.3-OCL veryclean

clean:  analysis_clean \
				opencl_runtime_clean \
				power_management_clean \
				load_balancer_clean \
				benchmarks_clean

# benchmarks 
.PHONY: analysis opencl_runtime power_management load_balancer all \
				analysis_clean opencl_runtime_clean power_management_clean \
				load_balancer_clean benchmarks_clean clean
