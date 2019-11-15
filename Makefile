INCLUDES += -I$(APPDIR)

APPLICATION = doriot_dca_test

BOARD ?= esp32-wroom-32
BOARD_WHITELIST = esp32-wroom-32 native

RIOTBASE ?= $(CURDIR)/../..

EXTERNAL_MODULE_DIRS += $(CURDIR)/module

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
USEMODULE += doriot_dca

DEVELHELP ?= 1

QUIET ?= 1

include $(RIOTBASE)/Makefile.include
