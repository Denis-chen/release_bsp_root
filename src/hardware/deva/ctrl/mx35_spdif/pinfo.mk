define PINFO
PINFO DESCRIPTION=Driver for the i.MX35 SPDIF controller
endef

SDMA_LIB=dma-sdma-imx35
LIBS_mx35_spdif += $(SDMA_LIB)
