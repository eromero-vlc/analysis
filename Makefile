# -DAMPPREFACTORS
# -DHAVE_DISPLIST
# -DPRINT_SVS

# -DMYDECOMP
# -DMYDECOMP2
# -DARDECOMP
# -DMARTHADECOMP
# -DDIAGMATS
# -DROWONE

EIGEN ?= /usr/include
HDF5 ?= /usr
HDF5_INCLUDE ?= -I$(HDF5)/include
HDF5_LIBS ?= -L$(HDF5)/lib -lhdf5 -lhdf5_cpp -lhdf5_hl -lhdf5_hl_cpp
GSL ?= /usr
GSL_INCLUDE ?= -I$(GSL)/include
GSL_LIBS ?= -L$(GSL)/lib -lgsl -lgslcblas -lm

PREFIX ?= $(PWD)
OPTS ?= -DMYDECOMP2 -DPRINT_SVS -DHAVE_DISPLIST -DVERBOSITY=1

CXXFLAGS_this := $(CXXFLAGS) $(OPTS) $(shell redstar-config --cxxflags ) -I$(PWD)/pPDF -I$(PWD)/fitting -I$(PWD)/corrUtil -I$(PWD)/linAlg -I$(PWD)/kine -I$(EIGEN) $(HDF5_INCLUDE) $(GSL_INCLUDE)
LDFLAGS_this := $(LDFLAGS) 
LIBS_this := $(shell redstar-config --ldflags ) $(shell redstar-config --libs ) $(HDF5_LIBS) $(GSL_LIBS) $(LIBS) 

SOURCES := \
	pPDF/operators.cc \
	pPDF/pseudo_utils.cc \
	fitting/fit_util.cc \
	fitting/varpro.cc \
	corrUtil/corr_utils.cc \
	linAlg/cov_utils.cc \
	kine/threept_tr.cc \
	kine/old_irrep_util_angles.cc \
	kine/rotations.cc 

#BINS := eval_kin_mat ff-matelem pGITD-matelem pITD-matelem
BINS := eval_kin_mat

install: create_dir $(BINS)

create_dir:
	install -d $(PREFIX)

eval_kin_mat: $(SOURCES) 
	$(CXX) $(CXXFLAGS_this) $(SOURCES) eval_kin_mat.cc $(LDFLAGS_this) $(LIBS_this) -o $(PREFIX)/eval_kin_mat 

ff-matelem: $(SOURCES)
	$(CXX) $(CXXFLAGS_this) -o $(PREFIX)/ff-matelem matelems/FF/ff-matelem.cc $(SOURCES) $(LDFLAGS_this) $(LIBS_this)

pGITD-matelem: $(SOURCES)
	$(CXX) $(CXXFLAGS_this) -o $(PREFIX)/pGITD-matelem matelems/pGITD/pGITD-matelem.cc $(SOURCES) $(LDFLAGS_this) $(LIBS_this)

pITD-matelem: $(SOURCES)
	$(CXX) $(CXXFLAGS_this) -o $(PREFIX)/pITD-matelem matelems/pITD/pITD-matelem.cc $(SOURCES) $(LDFLAGS_this) $(LIBS_this)

clean:
	rm -f $(BINS)
