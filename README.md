# git-auto-pull

[![Build Status](https://git.zjuqsc.com/ci/projects/3/status.png?ref=master)](https://git.zjuqsc.com/ci/projects/3)
A tool to handle GitLab and GitHub's Push event Webhook.

## Usage

 __NOTICE:__ Install from deb/rpm package directly will also install our apt/yum repo automatically for you. 

Install from make:
    
    ./make-libs # Download some libraries for static link
    make static64 # or make static32 for 32-bit systems
    sudo make install
    
NOTE: Install using make won't install any of our repos or public key files, and this software does not depend on the repos or keys.

1. Install from http://dl.zjuqsc.com/linux/
2. Set /etc/git-auto-pull/config.json
3. Enable and start ``git-auto-pull`` service
    
    # for systemd
    systemctl start git-auto-pull
    # for sysvinit
	service git-auto-pull
    

## Special
This software is now at a very early and unstable version. 

Use it AT YOUR OWN RISK. 

## License
GPLv3 or later

## Copyright
The files below:
- miscellaneous/config.json
- miscellaneous/git-auto-pull.service
- miscellaneous/git-auto-pull.initd
- miscellaneous/patch\_lib\_1.patch
- miscellaneous/patch\_lib\_2.patch
- main.c
- (Other files that have proper copyright header)

are copyrighted to Senorsen <sen@senorsen.com>.
