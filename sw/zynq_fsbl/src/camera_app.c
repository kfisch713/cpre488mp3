/*****************************************************************************
 * Joseph Zambreno
 * Phillip Jones
 *
 * Department of Electrical and Computer Engineering
 * Iowa State University
 *****************************************************************************/

/*****************************************************************************
 * camera_app.c - main camera application code. The camera configures the various
 * video in and video out peripherals, and (optionally) performs some
 * image processing on data coming in from the vdma.
 *
 *
 * NOTES:
 * 02/04/14 by JAZ::Design created.
 *****************************************************************************/

#include "camera_app.h"
#include <stdint.h>
camera_config_t camera_config;

/* Added for camera_interfaceing */
#define MAX_RAW_IMAGES 32
#define MODE_SWITCH 0
#define KILL_SWITCH 7

#define MAX_ZOOM_LVL 4

#define HEIGHT 1080
#define WIDTH 1920
#define FRAME_LEN (HEIGHT * WIDTH)


// Main function. Initializes the devices and configures VDMA
int camera_main() {
	camera_config_init(&camera_config);
	fmc_imageon_enable(&camera_config);

	Xuint32 parkptr;

	// Grab the DMA parkptr, and update it to ensure that when parked, the S2MM side is on frame 0, and the MM2S side on frame 1
	parkptr = XAxiVdma_ReadReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_PARKPTR_OFFSET);
	parkptr &= ~XAXIVDMA_PARKPTR_READREF_MASK;
	parkptr &= ~XAXIVDMA_PARKPTR_WRTREF_MASK;
	parkptr |= 0x1;
	XAxiVdma_WriteReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_PARKPTR_OFFSET, parkptr);

	Xuint32 vdma_S2MM_DMACR, vdma_MM2S_DMACR;

	// Grab the DMA Control Registers, and clear circular park mode.
	vdma_MM2S_DMACR = XAxiVdma_ReadReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_TX_OFFSET+XAXIVDMA_CR_OFFSET);
	XAxiVdma_WriteReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_TX_OFFSET+XAXIVDMA_CR_OFFSET, vdma_MM2S_DMACR & ~XAXIVDMA_CR_TAIL_EN_MASK);
	vdma_S2MM_DMACR = XAxiVdma_ReadReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_RX_OFFSET+XAXIVDMA_CR_OFFSET);
	XAxiVdma_WriteReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_RX_OFFSET+XAXIVDMA_CR_OFFSET, vdma_S2MM_DMACR & ~XAXIVDMA_CR_TAIL_EN_MASK);

	// Grab the DMA Control Registers, and re-enable circular park mode.
	vdma_MM2S_DMACR = XAxiVdma_ReadReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_TX_OFFSET+XAXIVDMA_CR_OFFSET);
	XAxiVdma_WriteReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_TX_OFFSET+XAXIVDMA_CR_OFFSET, vdma_MM2S_DMACR | XAXIVDMA_CR_TAIL_EN_MASK);
	vdma_S2MM_DMACR = XAxiVdma_ReadReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_RX_OFFSET+XAXIVDMA_CR_OFFSET);
	XAxiVdma_WriteReg(camera_config.vdma_hdmi.BaseAddr, XAXIVDMA_RX_OFFSET+XAXIVDMA_CR_OFFSET, vdma_S2MM_DMACR | XAXIVDMA_CR_TAIL_EN_MASK);

	return 0;
}

// Initialize the camera configuration data structure
void camera_config_init(camera_config_t *config) {
    config->uBaseAddr_IIC_FmcIpmi = XPAR_IIC_FMC_BASEADDR;
    config->uBaseAddr_IIC_FmcImageon = XPAR_FMC_IMAGEON_IIC_0_BASEADDR;
    config->uBaseAddr_VITA_Receiver = XPAR_FMC_IMAGEON_VITA_RECEIVER_0_BASEADDR;
    config->uBaseAddr_CFA = XPAR_CFA_0_BASEADDR;
    config->uBaseAddr_CRES = XPAR_CRESAMPLE_0_BASEADDR;
    config->uBaseAddr_RGBYCC = XPAR_RGB2YCRCB_0_BASEADDR;
    config->uBaseAddr_TPG_PatternGenerator = XPAR_AXI_TPG_0_BASEADDR;

    config->uDeviceId_VTC_ipipe = XPAR_V_TC_0_DEVICE_ID;
    config->uDeviceId_VTC_tpg   = XPAR_V_TC_1_DEVICE_ID;

    config->uDeviceId_VDMA_HdmiFrameBuffer = XPAR_AXI_VDMA_0_DEVICE_ID;
    config->uBaseAddr_MEM_HdmiFrameBuffer = XPAR_DDR_MEM_BASEADDR + 0x10000000;
    config->uNumFrames_HdmiFrameBuffer = XPAR_AXIVDMA_0_NUM_FSTORES;

    return;
}
