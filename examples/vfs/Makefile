INCLUDES += -I$(APPDIR)

APPLICATION = dca_vfs_example

BOARD ?= nucleo-f429zi

# Blacklisting msp430-based boards, as file syscalls are not supported
BOARD_BLACKLIST :=  chronos \
                    msb-430 \
                    msb-430h \
                    telosb \
                    wsn430-v1_3b \
                    wsn430-v1_4 \
                    z1 \
                    #

BOARD_INSUFFICIENT_MEMORY := arduino-duemilanove arduino-leonardo arduino-nano \
                             arduino-uno nucleo-f031k6

RIOTBASE ?= $(CURDIR)/../../../..

EXTERNAL_MODULE_DIRS += $(CURDIR)/../../..

ifeq ($(BOARD), esp32-wroom-32)
	USEMODULE += esp_wifi
	USEMODULE += esp_hw_counter
endif
USEMODULE += doriot_dca
USEMODULE += posix_inet
USEMODULE += core_idle_thread
USEMODULE += xtimer
USEMODULE += vfs
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

CFLAGS += -DCONFIG_GNRC_IPV6_NIB_ARSM=1
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_SLAAC=1
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_6LN=0
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_NUMOF=16
CFLAGS += -DVFS_DIR_BUFFER_SIZE=16 -DVFS_FILE_BUFFER_SIZE=16
CFLAGS += -DVFS_NAME_MAX=31
CFLAGS += -DDEBUG_ASSERT_VERBOSE

DEVELHELP ?= 1

QUIET ?= 1

include $(RIOTBASE)/Makefile.include
