#
# Copyright (C) Senorsen (Zhang Sen) <sen@senorsen.com>
#
# This software is distributed under the terms of GPLv3 or later,
# See `LICENSE' for more information. 
#

CRTS=github.com.crt git.zjuqsc.com.crt
CRT_SUFFIX=.crt
TARGETS := $(patsubst %$(CRT_SUFFIX),%,$(CRTS))

all: $(CRTS)

clean:
	rm -rf $(CRTS)

.SECONDEXPANSION:
CRT = $@
TARGET = $(patsubst %$(CRT_SUFFIX),%,$@)

%$(CRT_SUFFIX): $(CRTS)
	openssl s_client -showcerts -connect $(TARGET):443 < /dev/null | openssl x509 -outform PEM > $(CRT)

