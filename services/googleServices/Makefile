TOPDIR ?= ../..
include $(TOPDIR)/.config

$(GSERVICE_SUPPORT)_SRC += services/googleServices/googleservices.c

$(GWEATHER_SUPPORT)_SRC += services/googleServices/gweather/gweather.c
$(GWEATHER_SUPPORT)_ECMD_SRC += services/googleServices/gweather/gweather_ecmd.c

$(GCALENDAR_SUPPORT)_SRC += services/googleServices/gcalendar/gcalendar.c
$(GCALENDAR_SUPPORT)_ECMD_SRC += services/googleServices/gcalendar/gcalendar-ecmd.c

##############################################################################
# generic fluff
include $(TOPDIR)/scripts/rules.mk
