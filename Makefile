INCLUDES += -I$(APPDIR)

APPLICATION = doriot_dca_test

BOARD ?= esp32-wroom-32
BOARD_WHITELIST = esp32-wroom-32 native nucleo-f767zi msbiot nucleo-f429zi

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

RIOTBASE ?= $(CURDIR)/../..

EXTERNAL_MODULE_DIRS += $(CURDIR)/module

USE_DCAFS = 1

ifeq ($(BOARD), esp32-wroom-32)
	USEMODULE += esp_wifi
	USEMODULE += esp_hw_counter
endif
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += posix_time
USEMODULE += posix_inet
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += od
USEMODULE += fmt
USEMODULE += ps
USEMODULE += vfs
USEMODULE += schedstatistics
USEMODULE += auto_init
USEMODULE += core_idle_thread
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gnrc_ipv6_nib
USEMODULE += auto_init_gnrc_ipv6_nib
USEMODULE += gnrc_sock_udp
USEMODULE += xtimer
USEMODULE += saul_default

ifeq ($(USE_DCAFS), 1)
	USEMODULE += doriot_dca
	CFLAGS += -DUSE_DCAFS
endif

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
