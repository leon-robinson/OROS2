#pragma once

#include <types.h>

PACKED typedef enum APIC_TYPE {
	APIC_TYPE_LOCAL_APIC,
	APIC_TYPE_IO_APIC,
	APIC_TYPE_INTERRUPT_OVERRIDE
} APIC_TYPE;

PACKED typedef struct ACPI_RSDP {
	char signature[8];
	uint8 checksum;
	char oem_id[6];
	uint8 revision;
	uint32 rsdt_address;
} ACPI_RSDP;

PACKED typedef struct ACPI_HEADER {
	char signature[4];
	uint32 length;
	uint8 revision;
	uint8 checksum;
	uint8 oem_id[6];
	char oem_table_id[8];
	uint32 oem_revision;
	uint32 creator_id;
	uint32 creator_revision;
} ACPI_HEADER;

PACKED typedef struct APIC_HEADER {
	APIC_TYPE type;
	uint8 length;
} APIC_HEADER;

PACKED typedef struct MCFG_ENTRY {
	uint64 base_address;
	uint16 segment;
	uint8 start_bus;
	uint8 end_bus;
	uint32 reserved;
} MCFG_ENTRY;

PACKED typedef struct MCFG_HEADER {
	ACPI_HEADER header;
	uint64 reserved;
	MCFG_ENTRY entry_0;
} MCFG_HEADER;

PACKED typedef struct APIC_LOCAL_APIC {
	APIC_HEADER header;
	uint8 acpi_processor_id;
	uint8 apic_id;
	uint32 flags;
} APIC_LOCAL_APIC;

PACKED typedef struct APIC_IO_APIC {
	APIC_HEADER header;
	uint8 io_apic_id;
	uint8 reserved;
	uint32 io_apic_address;
	uint32 global_system_interrupt_base;
} APIC_IO_APIC;

PACKED typedef struct APIC_INTERRUPT_OVERRIDE {
	APIC_HEADER header;
	uint8 bus;
	uint8 source;
	uint32 interrupt;
	uint16 flags;
} APIC_INTERRUPT_OVERRIDE;

PACKED typedef struct ACPI_HPET_ADDRESS_STRUCTURE {
	uint8 address_space_id;
	uint8 register_bit_width;
	uint8 register_bit_offset;
	uint8 reserved;
	uint64 address;
} ACPI_HPET_ADDRESS_STRUCTURE;

PACKED typedef struct APIC_HPET {
	ACPI_HEADER header;
	uint8 hardware_revision_id;
	uint8 attribute;
	uint16 pci_vendor_id;
	ACPI_HPET_ADDRESS_STRUCTURE addresses;
	uint8 hpet_number;
	uint16 minimum_tick;
	uint8 page_protection;
} APIC_HPET;

PACKED typedef struct ACPI_FADT {
	ACPI_HEADER header;

	uint32 firmware_ctrl;
	uint32 dsdt;

	uint8 reserved;

	uint8 preferred_power_management_profile;
	uint16 sci_interrupt;
	uint32 smi_command_port;
	uint8 acpi_enable;
	uint8 acpi_disable;
	uint8 s4bios_req;
	uint8 pstate_control;
	uint32 pm1a_event_block;
	uint32 pm1b_event_block;
	uint32 pm1a_control_block;
	uint32 pm1b_control_block;
	uint32 pm2_control_block;
	uint32 pm_timer_block;
	uint32 gpe0_block;
	uint32 gpe1_block;
	uint8 pm1_event_length;
	uint8 pm1_control_length;
	uint8 pm2_control_length;
	uint8 pm_timer_length;
	uint8 gpe0_length;
	uint8 gpe1_length;
	uint8 gpe1_base;
	uint8 c_state_control;
	uint16 worst_c2_latency;
	uint16 worst_c3_latency;
	uint16 flush_size;
	uint16 flush_stride;
	uint8 duty_offset;
	uint8 duty_width;
	uint8 day_alarm;
	uint8 month_alarm;
	uint8 century;

	uint16 boot_architecture_flags;

	uint8 reserved2;
	uint32 flags;
} ACPI_FADT;

PACKED typedef struct ACPI_MADT {
	ACPI_HEADER header;
	uint32 local_apic_address;
	uint32 flags;
} ACPI_MADT;