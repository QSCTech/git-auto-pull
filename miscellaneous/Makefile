#
# Copyright (C) Senorsen (Zhang Sen) <sen@senorsen.com>
#
# This software is distributed under the terms of GPLv3 or later,
# See `LICENSE' for more information. 
#

all:
	openssl s_client -showcerts -connect github.com:443 < /dev/null | openssl x509 -outform PEM > github.com.crt
	openssl s_client -showcerts -connect git.zjuqsc.com:443 < /dev/null | openssl x509 -outform PEM > git.zjuqsc.com.crt

