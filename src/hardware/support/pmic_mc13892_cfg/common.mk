ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME=pmic_mc13892_cfg

USEFILE=$(PROJECT_ROOT)/$(NAME).c

INSTALLDIR = bin

define PINFO
PINFO DESCRIPTION=PMIC MC13892 POWER MANAGEMENT chip Configuration Utility
endef

include $(MKFILES_ROOT)/qtargets.mk
-include $(PROJECT_ROOT)/roots.mk
#####AUTO-GENERATED by packaging script... do not checkin#####
   INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../../install
   USE_INSTALL_ROOT=1
##############################################################
