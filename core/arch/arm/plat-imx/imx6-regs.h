/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 * All rights reserved.
 * Copyright (c) 2016, Wind River Systems.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define PERIPH_BASE			0x00A00000
#define PERIPH_SIZE			0x00002000

#define SCU_BASE			0x00A00000
#define PL310_BASE			0x00A02000
#define SRC_BASE			0x020D8000
#define SRC_SCR				0x000
#define SRC_GPR1			0x020
#define SRC_SCR_CPU_ENABLE_ALL		SHIFT_U32(0x7, 22)
#define SRC_SCR_CORE1_RST_OFFSET	14
#define SRC_SCR_CORE1_ENABLE_OFFSET	22
#define GIC_BASE			0x00A00000
#define GICC_OFFSET			0x100
#define GICD_OFFSET			0x1000
#define GIC_CPU_BASE			(GIC_BASE + GICC_OFFSET)
#define GIC_DIST_BASE			(GIC_BASE + GICD_OFFSET)
#define TZC380_BASE			0x021D0000
#define UART1_BASE			0x02020000
#define UART2_BASE			0x021E8000
#define UART4_BASE			0x021F0000

#define GT_BASE				0x00A00200
#define GT_COUNTER1			0x00
#define GT_COUNTER2			0x04
#define GT_CTRL				0x08
#define GT_ISR				0x0C
#define GT_CMPR1			0x10
#define GT_CMPR2			0x14
#define GT_AUTOINCR			0x18

#define APBHDMA_BASE			0x00110000
#define APBHDMA_SIZE			0x00002000
#define PCIE_BASE			0x01000000
#define PCIE_SIZE			0x00FFC000
#define HDMI_BASE			0x00120000
#define HDMI_SIZE			0x00009000
#define SATA_BASE			0x02200000
#define SATA_SIZE			0x00004000
#define VPU_BASE			0x02040000
#define VPU_SIZE			0x0003C000
#define GPU2D_BASE			0x00134000
#define GPU2D_SIZE			0x00004000
#define GPU3D_BASE			0x00130000
#define GPU3D_SIZE			0x00004000
#define IPU_BASE			0x02600000
#define IPU_SIZE			0x00800000
#define OPENVG_BASE			0x02204000
#define OPENVG_SIZE			0x00004000

#if defined(CFG_MX6Q) || defined(CFG_MX6D)
#define UART3_BASE			0x021EC000
#define UART5_BASE			0x021F4000
#endif

/* Central Security Unit register values */
#define CSU_BASE			0x021C0000
#define CSU_CSL_START			0x0
#define CSU_CSL0			0x00
#define CSU_CSL1			0x04
#define CSU_CSL2			0x08
#define CSU_CSL3			0x0C
#define CSU_CSL4			0x10
#define CSU_CSL5			0x14
#define CSU_CSL6			0x18
#define CSU_CSL7			0x1C
#define CSU_CSL8			0x20
#define CSU_CSL9			0x24
#define CSU_CSL10			0x28
#define CSU_CSL11			0x2C
#define CSU_CSL12			0x30
#define CSU_CSL13			0x34
#define CSU_CSL14			0x38
#define CSU_CSL15			0x3C
#define CSU_CSL16			0x40
#define CSU_CSL17			0x44
#define CSU_CSL18			0x48
#define CSU_CSL19			0x4C
#define CSU_CSL20			0x50
#define CSU_CSL21			0x54
#define CSU_CSL22			0x58
#define CSU_CSL23			0x5C
#define CSU_CSL24			0x60
#define CSU_CSL25			0x64
#define CSU_CSL26			0x68
#define CSU_CSL27			0x6C
#define CSU_CSL28			0x70
#define CSU_CSL29			0x74
#define CSU_CSL30			0x78
#define CSU_CSL31			0x7C
#define CSU_CSL32			0x80
#define CSU_CSL33			0x84
#define CSU_CSL34			0x88
#define CSU_CSL35			0x8C
#define CSU_CSL36			0x90
#define CSU_CSL37			0x94
#define CSU_CSL38			0x98
#define CSU_CSL39			0x9C
#define CSU_CSL_END			0xA0
#define CSU_HP0				0x200
#define CSU_HP1				0x204
#define CSU_SA				0x218
#define CSU_HPCTRL0			0x358
#define CSU_HPCTRL1			0x35C
#define CSU_ACCESS_NONE			0x00330033
#define CSU_ACCESS_A			0x003300FF
#define CSU_ACCESS_B			0x00FF0033
#define CSU_ACCESS_ALL			0x00FF00FF
#define CSU_SETTING_LOCK		0x01000100

#define AIPSTZ1_MPR			0x0207C000
#define AIPSTZ1_OPACR			0x0207C040
#define AIPSTZ1_OPACR1			0x0207C044
#define AIPSTZ1_OPACR2			0x0207C048
#define AIPSTZ1_OPACR3			0x0207C04C
#define AIPSTZ1_OPACR4			0x0207C050

#define AIPSTZ2_MPR			0x0217C000
#define AIPSTZ2_OPACR			0x0217C040
#define AIPSTZ2_OPACR1			0x0217C044
#define AIPSTZ2_OPACR2			0x0217C048
#define AIPSTZ2_OPACR3			0x0217C04C
#define AIPSTZ2_OPACR4			0x0217C050

#define DRAM0_BASE			0x10000000
