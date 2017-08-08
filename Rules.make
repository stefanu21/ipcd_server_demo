CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc

IFLAGS = -I$(TOPDIR)/include 
OFLAGS = -ggdb -O2 -fomit-frame-pointer
CPPFLAGS += -Wall -Wno-unused-local-typedefs $(IFLAGS) $(OFLAGS)
CFLAGS += $(CPPFLAGS) #use ptx incdir for libs

LIBS += -lm

SRCS += $(wildcard *.c) $(wildcard *.cpp)
HDRS += $(wildcard *.h)
HDRS += $(wildcard $(TOPDIR)/include/*.h)

.PHONY:		all all_rec clean clean_rec

ifdef SUBDIRS
all:		all_rec
else
all:		.depend $(TARGET) 
endif

all_rec:
ifdef SUBDIRS
		for i in $(SUBDIRS); do $(MAKE) -C $$i $(subst _rec,,$@) || exit 1; done
endif

$(TARGET):	$(OBJS)
		$(LD) -o $(TARGET) $(filter $(OBJS), $^) $(LDFLAGS) $(LIBS)

clean:		clean_gen clean_rec

clean_gen:
		$(RM) $(TARGET) $(OBJS) *.o .depend

clean_rec:
ifdef SUBDIRS
		for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done
endif

%.o:		%.c
		$(CC) -c $(CFLAGS) -o $@ $<

.depend:	$(SRCS) $(HDRS)
ifneq ($(strip $(SRCS)),)
		$(CC) -MM -MP -MG -MF .depend $(CFLAGS) $(SRCS)
endif

-include .depend
