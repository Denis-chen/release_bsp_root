ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=startup library
endef

ASMOFF_FORMAT_x86 = cpp

NAME = startup

DEFFILE = asmoff.def 

INSTALLDIR = usr/lib

MDRIVER_SRCPATH = $(MDRIVER_ROOT)/$(CPU) $(MDRIVER_ROOT)
EXTRA_SRCVPATH += $(if $(wildcard $(MDRIVER_ROOT)/mdriver.c),$(MDRIVER_SRCPATH))

-include ../../roots.mk
#####AUTO-GENERATED by packaging script... do not checkin#####
   INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../../install
   USE_INSTALL_ROOT=1
##############################################################
ifndef MDRIVER_ROOT
MDRIVER_ROOT=$(PRODUCT_ROOT)/mdriver
endif

VEND_ion=vendor
VEND_iox=vendor
VEND_qss=extra
VENDOR=$(VEND_$(firstword $(BUILDENV) qss))

PRE_SRCVPATH = $(if $(wildcard $(PROJECT_ROOT)/$(VENDOR)),$(PROJECT_ROOT)/$(VENDOR)/$(CPU) $(PROJECT_ROOT)/$(VENDOR))

include $(MKFILES_ROOT)/qtargets.mk

asmoff.def: $(PROJECT_ROOT)/asmoff.h $(CPU_ROOT)/asmoff.c

CCVFLAG_64 = -D_PADDR_BITS=64

CCFLAGS_gcc_ = -O2 -fomit-frame-pointer
CCFLAGS_gcc_qcc = -O2 -Wc,-fomit-frame-pointer
CCFLAGS_$(BUILDENV) = -DBUILDENV_$(BUILDENV)
CCFLAGS = $(CCFLAGS_$(COMPILER_TYPE)_$(COMPILER_DRIVER)) $(CCFLAGS_$(BUILDENV))

#
# Allow us to generate the deprecated sections
#
CCFLAGS += -DENABLE_DEPRECATED_SYSPAGE_SECTIONS

#
# This particular little kludge is to stop GCC from using F.P. instructions
# to move 8 byte quantities around. 
#
CC_nto_ppc_gcc += -msoft-float
CC_nto_ppc_gcc_qcc += -Wc,-msoft-float

callout_interrupt_mips_smp.o: callout_interrupt_mips_smp.S callout_interrupt_mips.S
callout_interrupt_85xxcpm.o: callout_interrupt_85xxcpm.s callout_interrupt_8260.s
