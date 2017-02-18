ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=ipl library
endef

ASFLAGS_x86 += -x assembler-with-cpp
ASFLAGS+=$(ASFLAGS_$(CPU))

NAME = ipl

## DEFFILE = asmoff.def 

INSTALLDIR = usr/lib

include $(MKFILES_ROOT)/qtargets.mk

-include $(PROJECT_ROOT)/roots.mk
#####AUTO-GENERATED by packaging script... do not checkin#####
   INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../../install
   USE_INSTALL_ROOT=1
##############################################################
#
# This particular little kludge is to stop GCC from using F.P. instructions
# to move 8 byte quantities around. 
#
CC_nto_ppc_gcc += -msoft-float
CC_nto_ppc_gcc_qcc += -Wc,-msoft-float
