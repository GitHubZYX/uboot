
#include <common.h>

#define FOTA_LEN			4096 
#define MISC_LEN			2048
#define IMAGE_FLAG			"IMAG"
#define FS_FLAG				"FILE"

struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[1024];
};

int check_recovery_misc(void)
{
	struct bootloader_message *boot;
	u8 *buf = (u8*)CONFIG_RAMDISK_LOADADDR;
	char *arg_cmd;
	char *arg_recovery;

	memset(buf, 0x00, MISC_LEN);
	flash_partition_read(CONFIG_PARTITION_MISC, buf, MISC_LEN);

	boot = (struct bootloader_message *)buf;

	if (boot->command[0] != 0 && boot->command[0] != 255)
		printf("Boot command: %.*s\n", sizeof(boot->command), boot->command);

	if (boot->status[0] != 0 && boot->status[0] != 255)
		printf("Boot status: %.*s\n", sizeof(boot->status), boot->status);

	boot->command[sizeof(boot->command) - 1] = '\0';  // Ensure termination
	boot->recovery[sizeof(boot->recovery) - 1] = '\0';  // Ensure termination
	arg_cmd = strtok(boot->command, "\n");
	arg_recovery = strtok(boot->recovery, "\n");
	if ((arg_recovery != NULL && !strcmp(arg_recovery, "recovery"))
		|| (arg_cmd != NULL && !strcmp(arg_cmd, "boot-recovery"))) {
		printf("do recovery(check misc) update...\n");
		return 1;
	}

	return 0;
}

int check_recovery_fota(void)
{
	u8 *buf = (u8*)CONFIG_RAMDISK_LOADADDR;

	memset(buf, 0x00, FOTA_LEN);
	flash_partition_read(CONFIG_PARTITION_FOTA, buf, FOTA_LEN);

	if ((strncmp((char *)(buf), FS_FLAG, 4) == 0)
		|| (strncmp((char *)(buf + 4), IMAGE_FLAG, 4) == 0)) {
		printf("do recovery(check fota) update...\n");
		return 1;
	}

	return 0;
}

