define PINFO
PINFO DESCRIPTION=Graphics driver dll for Freescale IMX35
endef
EXTRA_CCDEPS += $(PROJECT_ROOT)/$(SECTION)/imx35.h 

GCCVER:= $(if $(GCC_VERSION), $(GCC_VERSION), $(shell qcc -V 2>&1 | grep default | sed -e 's/,.*//'))

ifneq ($(filter 4.%, $(strip $(GCCVER))),)
	CCFLAGS += -mfpu=vfp -mfloat-abi=softfp
endif

SHARED_LIBS += m

#CCFLAGS += -DIMX35_SURFACE_FILL
#CCFLAGS += -DIMX35_KERNEL_TRACE
