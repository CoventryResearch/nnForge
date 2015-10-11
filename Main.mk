ifeq ($(USE_NNFORGE),yes)
ifeq ($(ENABLE_CUDA_BACKEND),yes)
LDLIBSDEPEND+=-lnnforge_cuda
GENERIC_CXXFLAGS+=-DNNFORGE_CUDA_BACKEND_ENABLED
endif
GENERIC_CXXFLAGS+=-I$(NNFORGE_PATH)
# LDLIBSDEPEND+=-lnnforge_plain -lnnforge
LDLIBSDEPEND+=-lnnforge
VPATH+=$(NNFORGE_PATH)/lib
LDFLAGS+=-L$(NNFORGE_PATH)/lib
endif

ifeq ($(USE_BOOST),yes)
ifneq ($(CPP11COMPILER),yes)
GENERIC_CXXFLAGS+=-I$(BOOST_PATH)/include/boost/tr1/tr1
endif
GENERIC_CXXFLAGS+=-I$(BOOST_PATH)/include
LDFLAGS+=-L$(BOOST_PATH)/lib $(BOOST_LIBS)
endif

ifeq ($(USE_PROTOBUF),yes)
GENERIC_CXXFLAGS+=-I$(PROTOBUF_PATH)/include
LDFLAGS+=-L$(PROTOBUF_PATH)/lib $(PROTOBUF_LIBS)
endif

ifeq ($(USE_NETCDF),yes)
ifneq ($(NETCDF_PATH),)
GENERIC_CXXFLAGS+=-I$(NETCDF_PATH)/include
LDFLAGS+=-L$(NETCDF_PATH)/lib
endif
LDFLAGS+=$(NETCDF_LIBS)
endif

ifeq ($(USE_MATIO),yes)
ifneq ($(MATIO_PATH),)
GENERIC_CXXFLAGS+=-I$(MATIO_PATH)/include
LDFLAGS+=-L$(MATIO_PATH)/lib
endif
LDFLAGS+=$(MATIO_LIBS)
endif

ifeq ($(USE_CUDA),yes)
GENERIC_CXXFLAGS+=-I$(CUDA_PATH)/include -I$(CUDNN_PATH)
LDFLAGS+=-L$(CUDA_PATH)/lib64 -L$(CUDA_PATH)/lib -L$(CUDNN_PATH) $(CUDA_LIBS)
endif

ifeq ($(USE_OPENCV),yes)
GENERIC_CXXFLAGS+=-I$(OPENCV_PATH)/include
LDFLAGS+=-L$(OPENCV_PATH)/lib $(OPENCV_LIBS)
endif

ifeq ($(USE_OPENMP),yes)
GENERIC_CXXFLAGS+=$(CPP_FLAGS_OPENMP)
LDFLAGS+=$(LD_FLAGS_OPENMP)
endif

GENERIC_CXXFLAGS+=$(CPP_FLAGS_COMMON)
ifeq ($(BUILD_MODE),debug)
GENERIC_CXXFLAGS+=$(CPP_FLAGS_DEBUG_MODE)
else
GENERIC_CXXFLAGS+=$(CPP_FLAGS_RELEASE_MODE)
endif

NVCCFLAGS=$(CUDA_FLAGS_COMMON)
ifeq ($(BUILD_MODE),debug)
NVCCFLAGS+=$(CUDA_FLAGS_DEBUG_MODE)
else
NVCCFLAGS+=$(CUDA_FLAGS_RELEASE_MODE)
endif

ifeq ($(ENABLE_CUDA_PROFILING),yes)
NVCCFLAGS+=-DENABLE_CUDA_PROFILING
GENERIC_CXXFLAGS+=-DENABLE_CUDA_PROFILING
LDFLAGS+=-lnvToolsExt
endif

NVCCFLAGS+=-Xcompiler="$(GENERIC_CXXFLAGS)"
CXXFLAGS=$(GENERIC_CXXFLAGS)

ifeq ($(CPP11COMPILER),yes)
NVCCFLAGS+=-DNNFORGE_CPP11COMPILER -std=c++11
CXXFLAGS+=$(CPP_FLAGS_CPP11)
CXXFLAGS+=-DNNFORGE_CPP11COMPILER
endif

ARFLAGS=rcvus

