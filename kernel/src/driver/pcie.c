#include <stdio.h>
#include "../pci.h"
#include "device.h"
#include "pcie.h"

int port_init(void* device, void* data) {
	return 0;
}

void port_destroy(int id) {
}

DeviceType port_device_type = DEVICE_TYPE_PORT;

Driver port_device_driver = {
	.init = port_init,
	.destroy = port_destroy
};

bool port_device_probe(PCI_Device* pci, char** name, void** data) {
	/* TODO: disabled temporary
	uint8_t type = pci_pcie_type(pci);
	if(!pci->caps[PCI_CAP_ID_EXP] && 
			(type != PCI_EXP_TYPE_ROOT_PORT &&
			type != PCI_EXP_TYPE_UPSTREAM &&
			type != PCI_EXP_TYPE_DOWNSTREAM))
		return false;
	
	pci_enable(pci);
	
	return true;
	*/
	return false;
}
