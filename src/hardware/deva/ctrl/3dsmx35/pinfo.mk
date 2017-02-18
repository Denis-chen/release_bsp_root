define PINFO
PINFO DESCRIPTION=Driver for the i.MX35 3DS audio controller
endef

SDMA_LIB=dma-sdma-imx35
LIBS_3dsmx35 += $(SDMA_LIB)
