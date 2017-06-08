#ifndef __PCI_H__
#define __PCI_H__

#include <stdint.h>
#include <stdbool.h>
#include "driver/pci.h"
#include "device.h"

// PCI Configuration Space
// Ref https://en.wikipedia.org/wiki/PCI_configuration_space#/media/File:Pci-config-space.svg
#define PCI_VENDOR_ID			0x00
#define PCI_DEVICE_ID			0x02
#define PCI_COMMAND			0x04
#define PCI_STATUS			0x06
#define PCI_REVISION_ID			0x08
#define PCI_PROG_IF			0x09
#define PCI_SUB_CLASS			0x0a
#define PCI_CLASS_CODE			0x0b
#define PCI_CACHE_LINE_SIZE		0x0c
#define PCI_LATENCY_TIMER		0x0d
#define PCI_HEADER_TYPE			0x0e
#define PCI_BIST			0x0f
#define PCI_BASE_ADDRESS_0		0x10
#define PCI_BASE_ADDRESS_1		0x14
#define PCI_BASE_ADDRESS_2		0x18
#define PCI_BASE_ADDRESS_3		0x1c
#define PCI_BASE_ADDRESS_4		0x20
#define PCI_BASE_ADDRESS_5		0x24
#define PCI_CARDBUS_CIS_POINTER		0x28
#define PCI_SUBSYSTEM_VENDOR_ID		0x2c
#define PCI_SUBSYSTEM_ID		0x2e
#define PCI_ROM_BASE_ADDRESS		0x30
#define PCI_CAPABILITY_LIST		0x34
#define PCI_INTERRUPT_LINE		0x3c
#define PCI_INTERRUPT_PIN		0x3d
#define PCI_MIN_GRANT			0x3e
#define PCI_MAX_LATENCY			0x3f

#define PCI_CB_CAPABILITY_LIST		0x14

#define PCI_HEADER_TYPE_NORMAL		0
#define PCI_HEADER_TYPE_BRIDGE		1
#define PCI_HEADER_TYPE_CARDBUS		2

#define PCI_BASE_ADDRESS_SPACE		0x01
#define PCI_BASE_ADDRESS_SPACE_IO	0x01
#define PCI_BASE_ADDRESS_SPACE_MEMORY	0x00
#define PCI_BASE_ADDRESS_MEM_TYPE_32	0x00
#define PCI_BASE_ADDRESS_MEM_TYPE_64	0x04
#define PCI_BASE_ADDRESS_MEM_MASK	(~0x0ful)
#define PCI_BASE_ADDRESS_IO_MASK	(~0x03ul)

#define PCI_COMMAND_IO			0x1	/* Enable response in I/O space */
#define PCI_COMMAND_MEMORY		0x2	/* Enable response in Memory space */
#define PCI_COMMAND_MASTER		0x4	/* Enable bus mastering */
#define PCI_COMMAND_SPECIAL		0x8	/* Enable response to special cycles */
#define PCI_COMMAND_INVALIDATE		0x10	/* Use memory write and invalidate */
#define PCI_COMMAND_VGA_PALETTE		0x20	/* Enable palette snooping */
#define PCI_COMMAND_PARITY		0x40	/* Enable parity checking */
#define PCI_COMMAND_WAIT		0x80	/* Enable address/data stepping */
#define PCI_COMMAND_SERR		0x100	/* Enable SERR */
#define PCI_COMMAND_FAST_BACK		0x200	/* Enable back-to-back writes */
#define PCI_COMMAND_INTX_DISABLE	0x400	/* INTx Emulation Disable */

#define PCI_STATUS_INTERRUPT		0x08	/* Interrupt status */
#define PCI_STATUS_CAP_LIST		0x10	/* Support Capability List */
#define PCI_STATUS_66MHZ		0x20	/* Support 66 Mhz PCI 2.1 bus */
#define PCI_STATUS_UDF 			0x40	/* Support User Definable Features [obsolete] */
#define PCI_STATUS_FAST_BACK		0x80	/* Accept fast-back to back */
#define PCI_STATUS_PARITY		0x100	/* Detected parity error */
#define PCI_STATUS_DEVSEL_MASK		0x600	/* DEVSEL timing */
#define PCI_STATUS_DEVSEL_FAST 		0x000
#define PCI_STATUS_DEVSEL_MEDIUM	0x200
#define PCI_STATUS_DEVSEL_SLOW 		0x400
#define PCI_STATUS_SIG_TARGET_ABORT	0x800	/* Set on target abort */
#define PCI_STATUS_REC_TARGET_ABORT	0x1000	/* Master ack of " */
#define PCI_STATUS_REC_MASTER_ABORT	0x2000	/* Set on master abort */
#define PCI_STATUS_SIG_SYSTEM_ERROR	0x4000	/* Set when we drive SERR */
#define PCI_STATUS_DETECTED_PARITY	0x8000	/* Set on parity error */

#define PCI_CAP_LIST_ID			0
#define PCI_CAP_LIST_NEXT		1
#define PCI_CAP_LIST_FLAGS		2
#define PCI_CAP_LIST_SIZEOF		4

#define PCI_CAP_ID_PM		0x01	/* Power Management */
#define PCI_CAP_ID_AGP		0x02	/* Accelerated Graphics Port */
#define PCI_CAP_ID_VPD		0x03	/* Vital Product Data */
#define PCI_CAP_ID_SLOTID	0x04	/* Slot Identification */
#define PCI_CAP_ID_MSI		0x05	/* Message Signalled Interrupts */
#define PCI_CAP_ID_CHSWP	0x06	/* CompactPCI HotSwap */
#define PCI_CAP_ID_PCIX		0x07	/* PCI-X */
#define PCI_CAP_ID_HT		0x08	/* HyperTransport */
#define PCI_CAP_ID_VNDR		0x09	/* Vendor specific */
#define PCI_CAP_ID_DBG		0x0A	/* Debug port */
#define PCI_CAP_ID_CCRC		0x0B	/* CompactPCI Central Resource Control */
#define PCI_CAP_ID_SHPC		0x0C	/* PCI Standard Hot-Plug Controller */
#define PCI_CAP_ID_SSVID	0x0D	/* Bridge subsystem vendor/device ID */
#define PCI_CAP_ID_AGP3		0x0E	/* AGP Target PCI-PCI bridge */
#define PCI_CAP_ID_SECDEV	0x0F	/* Secure Device */
#define PCI_CAP_ID_EXP		0x10	/* PCI Express */
#define PCI_CAP_ID_MSIX		0x11	/* MSI-X */
#define PCI_CAP_ID_SATA		0x12	/* SATA Data/Index Conf. */
#define PCI_CAP_ID_AF		0x13	/* PCI Advanced Features */
#define PCI_CAP_ID_MAX		PCI_CAP_ID_AF

#define PCI_D0		0
#define PCI_D1		1
#define PCI_D2		2
#define PCI_D3hot	3
#define PCI_D3cold	4

/* PCI Express */
#define PCI_EXP_FLAGS           2       /* Capabilities register */
#define PCI_EXP_FLAGS_VERS      0x000f  /* Capability version */
#define PCI_EXP_FLAGS_TYPE      0x00f0  /* Device/Port type */
#define  PCI_EXP_TYPE_ENDPOINT  0x0     /* Express Endpoint */
#define  PCI_EXP_TYPE_LEG_END   0x1     /* Legacy Endpoint */
#define  PCI_EXP_TYPE_ROOT_PORT 0x4     /* Root Port */
#define  PCI_EXP_TYPE_UPSTREAM  0x5     /* Upstream Port */
#define  PCI_EXP_TYPE_DOWNSTREAM 0x6    /* Downstream Port */
#define  PCI_EXP_TYPE_PCI_BRIDGE 0x7    /* PCI/PCI-X Bridge */
#define  PCI_EXP_TYPE_PCIE_BRIDGE 0x8   /* PCI/PCI-X to PCIE Bridge */
#define  PCI_EXP_TYPE_RC_END    0x9     /* Root Complex Integrated Endpoint */
#define  PCI_EXP_TYPE_RC_EC     0xa     /* Root Complex Event Collector */
#define PCI_EXP_FLAGS_SLOT      0x0100  /* Slot implemented */
#define PCI_EXP_FLAGS_IRQ       0x3e00  /* Interrupt message number */
#define PCI_EXP_DEVCAP          4       /* Device capabilities */
#define  PCI_EXP_DEVCAP_PAYLOAD 0x07    /* Max_Payload_Size */
#define  PCI_EXP_DEVCAP_PHANTOM 0x18    /* Phantom functions */
#define  PCI_EXP_DEVCAP_EXT_TAG 0x20    /* Extended tags */
#define  PCI_EXP_DEVCAP_L0S     0x1c0   /* L0s Acceptable Latency */
#define  PCI_EXP_DEVCAP_L1      0xe00   /* L1 Acceptable Latency */
#define  PCI_EXP_DEVCAP_ATN_BUT 0x1000  /* Attention Button Present */
#define  PCI_EXP_DEVCAP_ATN_IND 0x2000  /* Attention Indicator Present */
#define  PCI_EXP_DEVCAP_PWR_IND 0x4000  /* Power Indicator Present */
#define  PCI_EXP_DEVCAP_RBER    0x8000  /* Role-Based Error Reporting */
#define  PCI_EXP_DEVCAP_PWR_VAL 0x3fc0000 /* Slot Power Limit Value */
#define  PCI_EXP_DEVCAP_PWR_SCL 0xc000000 /* Slot Power Limit Scale */
#define  PCI_EXP_DEVCAP_FLR     0x10000000 /* Function Level Reset */
#define PCI_EXP_DEVCTL          8       /* Device Control */
#define  PCI_EXP_DEVCTL_CERE    0x0001  /* Correctable Error Reporting En. */
#define  PCI_EXP_DEVCTL_NFERE   0x0002  /* Non-Fatal Error Reporting Enable */
#define  PCI_EXP_DEVCTL_FERE    0x0004  /* Fatal Error Reporting Enable */
#define  PCI_EXP_DEVCTL_URRE    0x0008  /* Unsupported Request Reporting En. */
#define  PCI_EXP_DEVCTL_RELAX_EN 0x0010 /* Enable relaxed ordering */
#define  PCI_EXP_DEVCTL_PAYLOAD 0x00e0  /* Max_Payload_Size */
#define  PCI_EXP_DEVCTL_EXT_TAG 0x0100  /* Extended Tag Field Enable */
#define  PCI_EXP_DEVCTL_PHANTOM 0x0200  /* Phantom Functions Enable */
#define  PCI_EXP_DEVCTL_AUX_PME 0x0400  /* Auxiliary Power PM Enable */
#define  PCI_EXP_DEVCTL_NOSNOOP_EN 0x0800  /* Enable No Snoop */
#define  PCI_EXP_DEVCTL_READRQ  0x7000  /* Max_Read_Request_Size */
#define  PCI_EXP_DEVCTL_BCR_FLR 0x8000  /* Bridge Configuration Retry / FLR */
#define PCI_EXP_DEVSTA          10      /* Device Status */
#define  PCI_EXP_DEVSTA_CED     0x01    /* Correctable Error Detected */
#define  PCI_EXP_DEVSTA_NFED    0x02    /* Non-Fatal Error Detected */
#define  PCI_EXP_DEVSTA_FED     0x04    /* Fatal Error Detected */
#define  PCI_EXP_DEVSTA_URD     0x08    /* Unsupported Request Detected */
#define  PCI_EXP_DEVSTA_AUXPD   0x10    /* AUX Power Detected */
#define  PCI_EXP_DEVSTA_TRPND   0x20    /* Transactions Pending */
#define PCI_EXP_LNKCAP          12      /* Link Capabilities */
#define  PCI_EXP_LNKCAP_SLS     0x0000000f /* Supported Link Speeds */
#define  PCI_EXP_LNKCAP_SLS_2_5GB 0x1   /* LNKCAP2 SLS Vector bit 0 (2.5GT/s) */
#define  PCI_EXP_LNKCAP_SLS_5_0GB 0x2   /* LNKCAP2 SLS Vector bit 1 (5.0GT/s) */
#define  PCI_EXP_LNKCAP_MLW     0x000003f0 /* Maximum Link Width */
#define  PCI_EXP_LNKCAP_ASPMS   0x00000c00 /* ASPM Support */
#define  PCI_EXP_LNKCAP_L0SEL   0x00007000 /* L0s Exit Latency */
#define  PCI_EXP_LNKCAP_L1EL    0x00038000 /* L1 Exit Latency */
#define  PCI_EXP_LNKCAP_CLKPM   0x00040000 /* L1 Clock Power Management */
#define  PCI_EXP_LNKCAP_SDERC   0x00080000 /* Surprise Down Error Reporting Capable */
#define  PCI_EXP_LNKCAP_DLLLARC 0x00100000 /* Data Link Layer Link Active Reporting Capable */
#define  PCI_EXP_LNKCAP_LBNC    0x00200000 /* Link Bandwidth Notification Capability */
#define  PCI_EXP_LNKCAP_PN      0xff000000 /* Port Number */
#define PCI_EXP_LNKCTL          16      /* Link Control */
#define  PCI_EXP_LNKCTL_ASPMC   0x0003  /* ASPM Control */
#define  PCI_EXP_LNKCTL_ASPM_L0S  0x01  /* L0s Enable */
#define  PCI_EXP_LNKCTL_ASPM_L1   0x02  /* L1 Enable */
#define  PCI_EXP_LNKCTL_RCB     0x0008  /* Read Completion Boundary */
#define  PCI_EXP_LNKCTL_LD      0x0010  /* Link Disable */
#define  PCI_EXP_LNKCTL_RL      0x0020  /* Retrain Link */
#define  PCI_EXP_LNKCTL_CCC     0x0040  /* Common Clock Configuration */
#define  PCI_EXP_LNKCTL_ES      0x0080  /* Extended Synch */
#define  PCI_EXP_LNKCTL_CLKREQ_EN 0x100 /* Enable clkreq */
#define  PCI_EXP_LNKCTL_HAWD    0x0200  /* Hardware Autonomous Width Disable */
#define  PCI_EXP_LNKCTL_LBMIE   0x0400  /* Link Bandwidth Management Interrupt Enable */
#define  PCI_EXP_LNKCTL_LABIE   0x0800  /* Lnk Autonomous Bandwidth Interrupt Enable */
#define PCI_EXP_LNKSTA          18      /* Link Status */
#define  PCI_EXP_LNKSTA_CLS     0x000f  /* Current Link Speed */
#define  PCI_EXP_LNKSTA_CLS_2_5GB 0x01  /* Current Link Speed 2.5GT/s */
#define  PCI_EXP_LNKSTA_CLS_5_0GB 0x02  /* Current Link Speed 5.0GT/s */
#define  PCI_EXP_LNKSTA_NLW     0x03f0  /* Nogotiated Link Width */
#define  PCI_EXP_LNKSTA_NLW_SHIFT 4     /* start of NLW mask in link status */
#define  PCI_EXP_LNKSTA_LT      0x0800  /* Link Training */
#define  PCI_EXP_LNKSTA_SLC     0x1000  /* Slot Clock Configuration */
#define  PCI_EXP_LNKSTA_DLLLA   0x2000  /* Data Link Layer Link Active */
#define  PCI_EXP_LNKSTA_LBMS    0x4000  /* Link Bandwidth Management Status */
#define  PCI_EXP_LNKSTA_LABS    0x8000  /* Link Autonomous Bandwidth Status */
#define PCI_CAP_EXP_ENDPOINT_SIZEOF_V1  20      /* v1 endpoints end here */
#define PCI_EXP_SLTCAP          20      /* Slot Capabilities */
#define  PCI_EXP_SLTCAP_ABP     0x00000001 /* Attention Button Present */
#define  PCI_EXP_SLTCAP_PCP     0x00000002 /* Power Controller Present */
#define  PCI_EXP_SLTCAP_MRLSP   0x00000004 /* MRL Sensor Present */
#define  PCI_EXP_SLTCAP_AIP     0x00000008 /* Attention Indicator Present */
#define  PCI_EXP_SLTCAP_PIP     0x00000010 /* Power Indicator Present */
#define  PCI_EXP_SLTCAP_HPS     0x00000020 /* Hot-Plug Surprise */
#define  PCI_EXP_SLTCAP_HPC     0x00000040 /* Hot-Plug Capable */
#define  PCI_EXP_SLTCAP_SPLV    0x00007f80 /* Slot Power Limit Value */
#define  PCI_EXP_SLTCAP_SPLS    0x00018000 /* Slot Power Limit Scale */
#define  PCI_EXP_SLTCAP_EIP     0x00020000 /* Electromechanical Interlock Present */
#define  PCI_EXP_SLTCAP_NCCS    0x00040000 /* No Command Completed Support */
#define  PCI_EXP_SLTCAP_PSN     0xfff80000 /* Physical Slot Number */
#define PCI_EXP_SLTCTL          24      /* Slot Control */
#define  PCI_EXP_SLTCTL_ABPE    0x0001  /* Attention Button Pressed Enable */
#define  PCI_EXP_SLTCTL_PFDE    0x0002  /* Power Fault Detected Enable */
#define  PCI_EXP_SLTCTL_MRLSCE  0x0004  /* MRL Sensor Changed Enable */
#define  PCI_EXP_SLTCTL_PDCE    0x0008  /* Presence Detect Changed Enable */
#define  PCI_EXP_SLTCTL_CCIE    0x0010  /* Command Completed Interrupt Enable */
#define  PCI_EXP_SLTCTL_HPIE    0x0020  /* Hot-Plug Interrupt Enable */
#define  PCI_EXP_SLTCTL_AIC     0x00c0  /* Attention Indicator Control */
#define  PCI_EXP_SLTCTL_PIC     0x0300  /* Power Indicator Control */
#define  PCI_EXP_SLTCTL_PCC     0x0400  /* Power Controller Control */
#define  PCI_EXP_SLTCTL_EIC     0x0800  /* Electromechanical Interlock Control */
#define  PCI_EXP_SLTCTL_DLLSCE  0x1000  /* Data Link Layer State Changed Enable */
#define PCI_EXP_SLTSTA          26      /* Slot Status */
#define  PCI_EXP_SLTSTA_ABP     0x0001  /* Attention Button Pressed */
#define  PCI_EXP_SLTSTA_PFD     0x0002  /* Power Fault Detected */
#define  PCI_EXP_SLTSTA_MRLSC   0x0004  /* MRL Sensor Changed */
#define  PCI_EXP_SLTSTA_PDC     0x0008  /* Presence Detect Changed */
#define  PCI_EXP_SLTSTA_CC      0x0010  /* Command Completed */
#define  PCI_EXP_SLTSTA_MRLSS   0x0020  /* MRL Sensor State */
#define  PCI_EXP_SLTSTA_PDS     0x0040  /* Presence Detect State */
#define  PCI_EXP_SLTSTA_EIS     0x0080  /* Electromechanical Interlock Status */
#define  PCI_EXP_SLTSTA_DLLSC   0x0100  /* Data Link Layer State Changed */
#define PCI_EXP_RTCTL           28      /* Root Control */
#define  PCI_EXP_RTCTL_SECEE    0x01    /* System Error on Correctable Error */
#define  PCI_EXP_RTCTL_SENFEE   0x02    /* System Error on Non-Fatal Error */
#define  PCI_EXP_RTCTL_SEFEE    0x04    /* System Error on Fatal Error */
#define  PCI_EXP_RTCTL_PMEIE    0x08    /* PME Interrupt Enable */
#define  PCI_EXP_RTCTL_CRSSVE   0x10    /* CRS Software Visibility Enable */
#define PCI_EXP_RTCAP           30      /* Root Capabilities */
#define PCI_EXP_RTSTA           32      /* Root Status */
#define PCI_EXP_RTSTA_PME       0x10000 /* PME status */
#define PCI_EXP_RTSTA_PENDING   0x20000 /* PME pending */
/*
 * Note that the following PCI Express 'Capability Structure' registers
 * were introduced with 'Capability Version' 0x2 (v2).  These registers
 * do not exist on devices with Capability Version 1.  Use pci_pcie_cap2()
 * to use these fields safely.
 */
#define PCI_EXP_DEVCAP2         36      /* Device Capabilities 2 */
#define  PCI_EXP_DEVCAP2_ARI    0x20    /* Alternative Routing-ID */
#define  PCI_EXP_DEVCAP2_LTR    0x800   /* Latency tolerance reporting */
#define  PCI_EXP_OBFF_MASK      0xc0000 /* OBFF support mechanism */
#define  PCI_EXP_OBFF_MSG       0x40000 /* New message signaling */
#define  PCI_EXP_OBFF_WAKE      0x80000 /* Re-use WAKE# for OBFF */
#define PCI_EXP_DEVCTL2         40      /* Device Control 2 */
#define  PCI_EXP_DEVCTL2_ARI    0x20    /* Alternative Routing-ID */
#define  PCI_EXP_IDO_REQ_EN     0x100   /* ID-based ordering request enable */
#define  PCI_EXP_IDO_CMP_EN     0x200   /* ID-based ordering completion enable */
#define  PCI_EXP_LTR_EN         0x400   /* Latency tolerance reporting */
#define  PCI_EXP_OBFF_MSGA_EN   0x2000  /* OBFF enable with Message type A */
#define  PCI_EXP_OBFF_MSGB_EN   0x4000  /* OBFF enable with Message type B */
#define  PCI_EXP_OBFF_WAKE_EN   0x6000  /* OBFF using WAKE# signaling */
#define PCI_CAP_EXP_ENDPOINT_SIZEOF_V2  44      /* v2 endpoints end here */
#define PCI_EXP_LNKCAP2         44      /* Link Capability 2 */
#define  PCI_EXP_LNKCAP2_SLS_2_5GB 0x02 /* Supported Link Speed 2.5GT/s */
#define  PCI_EXP_LNKCAP2_SLS_5_0GB 0x04 /* Supported Link Speed 5.0GT/s */
#define  PCI_EXP_LNKCAP2_SLS_8_0GB 0x08 /* Supported Link Speed 8.0GT/s */
#define  PCI_EXP_LNKCAP2_CROSSLINK 0x100 /* Crosslink supported */
#define PCI_EXP_LNKCTL2         48      /* Link Control 2 */
#define PCI_EXP_LNKSTA2         50      /* Link Status 2 */
#define PCI_EXP_SLTCTL2         56      /* Slot Control 2 */

#define PCI_EXT_CAP_ID_ERR      0x01    /* Advanced Error Reporting */
#define PCI_EXT_CAP_ID_VC       0x02    /* Virtual Channel Capability */
#define PCI_EXT_CAP_ID_DSN      0x03    /* Device Serial Number */
#define PCI_EXT_CAP_ID_PWR      0x04    /* Power Budgeting */
#define PCI_EXT_CAP_ID_RCLD     0x05    /* Root Complex Link Declaration */
#define PCI_EXT_CAP_ID_RCILC    0x06    /* Root Complex Internal Link Control */
#define PCI_EXT_CAP_ID_RCEC     0x07    /* Root Complex Event Collector */
#define PCI_EXT_CAP_ID_MFVC     0x08    /* Multi-Function VC Capability */
#define PCI_EXT_CAP_ID_VC9      0x09    /* same as _VC */
#define PCI_EXT_CAP_ID_RCRB     0x0A    /* Root Complex RB? */
#define PCI_EXT_CAP_ID_VNDR     0x0B    /* Vendor Specific */
#define PCI_EXT_CAP_ID_CAC      0x0C    /* Config Access - obsolete */
#define PCI_EXT_CAP_ID_ACS      0x0D    /* Access Control Services */
#define PCI_EXT_CAP_ID_ARI      0x0E    /* Alternate Routing ID */
#define PCI_EXT_CAP_ID_ATS      0x0F    /* Address Translation Services */
#define PCI_EXT_CAP_ID_SRIOV    0x10    /* Single Root I/O Virtualization */
#define PCI_EXT_CAP_ID_MRIOV    0x11    /* Multi Root I/O Virtualization */
#define PCI_EXT_CAP_ID_MCAST    0x12    /* Multicast */
#define PCI_EXT_CAP_ID_PRI      0x13    /* Page Request Interface */
#define PCI_EXT_CAP_ID_AMD_XXX  0x14    /* reserved for AMD */
#define PCI_EXT_CAP_ID_REBAR    0x15    /* resizable BAR */
#define PCI_EXT_CAP_ID_DPA      0x16    /* dynamic power alloc */
#define PCI_EXT_CAP_ID_TPH      0x17    /* TPH request */
#define PCI_EXT_CAP_ID_LTR      0x18    /* latency tolerance reporting */
#define PCI_EXT_CAP_ID_SECPCI   0x19    /* Secondary PCIe */
#define PCI_EXT_CAP_ID_PMUX     0x1A    /* Protocol Multiplexing */
#define PCI_EXT_CAP_ID_PASID    0x1B    /* Process Address Space ID */
#define PCI_EXT_CAP_ID_MAX      PCI_EXT_CAP_ID_PASID

#define PCI_EXT_CAP_DSN_SIZEOF  12
#define PCI_EXT_CAP_MCAST_ENDPOINT_SIZEOF 40

#define PCIE_LINK_STATE_L0S     1
#define PCIE_LINK_STATE_L1      2
#define PCIE_LINK_STATE_CLKPM   4

typedef struct pci_dev {
	Device		dev;	// for Linux compabibility  
	
	char*		name;
	
	uint16_t	vendor_id;
	uint16_t	device_id;
	uint16_t	subvendor_id;
	uint16_t	subdevice_id;
	
	uint8_t		bus;
	uint8_t		slot;
	uint8_t		function;
	
	uint8_t		irq;
	
	uint8_t		caps[PCI_CAP_ID_MAX + 1];	// PCI capabilities
	uint16_t	capse[PCI_EXT_CAP_ID_MAX + 1];	// PCIe capabilities
	
	void*		priv;

	void* 		driver;
} PCI_Device;

typedef struct {
	uint16_t	id;
	uint8_t		ver: 4;
	uint16_t	next: 12;
	uint8_t		data[0];
} __attribute__((packed)) PCIeExtendedCapability;

#define PCI_MAX_DEVICES		32
#define PCI_MAX_BUS		256
#define PCI_MAX_SLOT		32
#define PCI_MAX_FUNCTION	8

extern PCI_Device pci_devices[PCI_MAX_DEVICES];
extern int pci_devices_count;
extern void* pci_mmio[PCI_MAX_BUS];

void pci_init();

uint32_t pci_device_size(uint16_t vendor_id, uint16_t device_id);
PCI_Device* pci_device(uint16_t vendor_id, uint16_t device_id, uint32_t index);
bool pci_enable(PCI_Device* device);
bool pci_power_set(PCI_Device* device);

uint8_t pci_read8(PCI_Device* device, uint32_t reg);
void pci_write8(PCI_Device* device, uint32_t reg, uint8_t data);
uint16_t pci_read16(PCI_Device* device, uint32_t reg);
void pci_write16(PCI_Device* device, uint32_t reg, uint16_t data);
uint32_t pci_read32(PCI_Device* device, uint32_t reg);
void pci_write32(PCI_Device* device, uint32_t reg, uint32_t data);

uint8_t pci_pcie_ver(PCI_Device* device);
//uint8_t pci_pcie_type(PCI_Device* device);

int pci_probe(DeviceType type, PCI_DEVICE_PROBE probe, Driver* driver);

typedef struct _PCI_Function_Entry {
	uint8_t function;
} PCI_Function_Entry;

typedef struct _PCI_Slot_Entry {
	uint8_t slot;
	uint8_t function_count;
	PCI_Function_Entry function_entry[PCI_MAX_FUNCTION];
} PCI_Slot_Entry;

typedef struct _PCI_Bus_Entry {
	uint8_t bus;
	uint8_t slot_count;
	PCI_Slot_Entry slot_entry[PCI_MAX_SLOT];
} PCI_Bus_Entry;

int pci_get_entrys(PCI_Bus_Entry* bus_entry);

#define PCI_DUMP_LEVEL_X		1
#define PCI_DUMP_LEVEL_XX		2
#define PCI_DUMP_LEVEL_XXX		3
#define PCI_DUMP_LEVEL_XXXX		4
void pci_data_dump(uint8_t bus, uint8_t slot, uint8_t function, uint8_t level);

#endif /* __PCI_H__ */
