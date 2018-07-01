export ARCH=arm
export PLATFORM=imx-mx6qsabrelite

export CFG_TEE_CORE_LOG_LEVEL=4
export DEBUG=y

export CFG_BUILT_IN_ARGS=y 
export CFG_PAGEABLE_ADDR=0 
export CFG_NS_ENTRY_ADDR=0x10800000 
export CFG_DT=y 
export CFG_DT_ADDR=0x13000000 

export CFG_BOOT_SYNC_CPU=n
export CFG_BOOT_SECONDARY_REQUEST=y
export CFG_PSCI_ARM32=y

export CFG_IMG_ADDR=0x4bffffe4
export CFG_IMG_ENTRY=0x4c000000
