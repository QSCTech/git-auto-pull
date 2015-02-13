# git-auto-pull

A tool to handle GitLab and GitHub's Push event Webhook.

## Usage

 __NOTICE:__ Install from deb/rpm package directly will also install our apt/yum repo automatically for you. 

Install from make:
    
    ./make-libs # Download some libraries for static link
    make static64 # or make static32 for 32-bit systems
    sudo make install
    
NOTE: Install using make won't install any of our repos or public key files, unless using ``make WITH_REPO=yes deb rpm``. 

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

