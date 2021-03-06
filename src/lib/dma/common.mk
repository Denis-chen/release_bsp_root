ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=DMA engine driver
endef

ifndef USEFILE
USEFILE=$(PROJECT_ROOT)/$(SECTION)/dma.use
endif
EXTRA_SRCVPATH += $(EXTRA_SRCVPATH_$(SECTION))
PUBLIC_INCVPATH += $(PROJECT_ROOT)/public


PRE_SRCVPATH += $(foreach var,$(filter a, $(VARIANTS)),$(CPU_ROOT)/$(subst $(space),.,$(patsubst a,dll,$(filter-out g, $(VARIANTS)))))

EXTRA_SILENT_VARIANTS = $(subst -, ,$(SECTION))
NAME=$(PROJECT)-$(SECTION)
include $(MKFILES_ROOT)/qmacros.mk
-include $(PROJECT_ROOT)/roots.mk
#####AUTO-GENERATED by packaging script... do not checkin#####
   INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../install
   USE_INSTALL_ROOT=1
##############################################################
-include $(PROJECT_ROOT)/$(SECTION)/pinfo.mk
include $(MKFILES_ROOT)/qtargets.mk
