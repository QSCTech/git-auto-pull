#!/bin/sh

echo "Summary: GitLab and GitHub Webhook auto pull service. Qiu Shi Chao Website. " > git-auto-pull.spec
echo "Name: git-auto-pull" >> git-auto-pull.spec
echo "Version: $1" >> git-auto-pull.spec
echo "Release: 1" >> git-auto-pull.spec
echo "License: GPL" >> git-auto-pull.spec
echo "Group: Development/Tools" >> git-auto-pull.spec
echo "URL: https://blog.senorsen.com" >> git-auto-pull.spec
echo "Packager: Senorsen <sen@senorsen.com>" >> git-auto-pull.spec
echo "Requires: git" >> git-auto-pull.spec
echo "BuildRoot:  %{_builddir}/%{name}-root" >> git-auto-pull.spec
echo "Source: %{name}-%{version}.tar.gz" >> git-auto-pull.spec
echo "" >> git-auto-pull.spec
echo "%description" >> git-auto-pull.spec
echo "GitLab and GitHub Webhook auto pull service. Qiu Shi Chao Website. " >> git-auto-pull.spec
echo "" >> git-auto-pull.spec
echo "%prep" >> git-auto-pull.spec
echo "%setup -q" >> git-auto-pull.spec
echo "" >> git-auto-pull.spec
echo "%build" >> git-auto-pull.spec
echo 'echo $RPM_BUILD' >> git-auto-pull.spec
echo "" >> git-auto-pull.spec
echo "%install" >> git-auto-pull.spec
echo 'rm -rf $RPM_BUILD_ROOT' >> git-auto-pull.spec
echo 'mkdir -p $RPM_BUILD_ROOT/usr/sbin' >> git-auto-pull.spec
echo 'mkdir -p $RPM_BUILD_ROOT/etc/git-auto-pull' >> git-auto-pull.spec
echo 'mkdir -p $RPM_BUILD_ROOT/var/log/git-auto-pull' >> git-auto-pull.spec
echo 'mkdir -p $RPM_BUILD_ROOT/usr/share/git-auto-pull' >> git-auto-pull.spec
echo 'mkdir -p $RPM_BUILD_ROOT/etc/init.d' >> git-auto-pull.spec
echo 'mkdir -p $RPM_BUILD_ROOT/lib/systemd/system' >> git-auto-pull.spec
echo 'mkdir -p $RPM_BUILD_ROOT/etc/pki/rpm-gpg' >> git-auto-pull.spec
echo 'mkdir -p $RPM_BUILD_ROOT/etc/yum.repos.d' >> git-auto-pull.spec
echo 'install -s -m 755 git-auto-pull.bin $RPM_BUILD_ROOT/usr/sbin/git-auto-pull' >> git-auto-pull.spec
echo 'install -m 644 config.json $RPM_BUILD_ROOT/etc/git-auto-pull/config.json' >> git-auto-pull.spec
echo 'install -m 755 git-auto-pull.initd $RPM_BUILD_ROOT/etc/init.d/git-auto-pull' >> git-auto-pull.spec
echo 'install -m 755 git-auto-pull.service $RPM_BUILD_ROOT/lib/systemd/system/git-auto-pull.service' >> git-auto-pull.spec
echo 'install -m 755 git-auto-pull.initd $RPM_BUILD_ROOT/usr/share/git-auto-pull/git-auto-pull' >> git-auto-pull.spec
echo 'install -m 755 git-auto-pull.service $RPM_BUILD_ROOT/usr/share/git-auto-pull/git-auto-pull.service' >> git-auto-pull.spec
echo 'install -d $RPM_BUILD_ROOT/var/log/git-auto-pull -o root -m 755' >> git-auto-pull.spec
echo 'install -m 644 qsc.public.key $RPM_BUILD_ROOT/usr/share/git-auto-pull/qsc.public.key' >> git-auto-pull.spec
echo 'install -m 644 qsc.repo $RPM_BUILD_ROOT/usr/share/git-auto-pull/qsc.repo' >> git-auto-pull.spec
echo "" >> git-auto-pull.spec
echo "%files" >> git-auto-pull.spec
echo "%defattr(-,root,root)" >> git-auto-pull.spec
echo "%config /etc/git-auto-pull/config.json" >> git-auto-pull.spec
echo "%dir /var/log/git-auto-pull" >> git-auto-pull.spec
echo "/usr/sbin/git-auto-pull" >> git-auto-pull.spec
echo "/usr/share/git-auto-pull/git-auto-pull" >> git-auto-pull.spec
echo "/usr/share/git-auto-pull/git-auto-pull.service" >> git-auto-pull.spec
echo "/usr/share/git-auto-pull/qsc.public.key" >> git-auto-pull.spec
echo "/usr/share/git-auto-pull/qsc.repo" >> git-auto-pull.spec
echo "/etc/init.d/git-auto-pull" >> git-auto-pull.spec
echo "/lib/systemd/system/git-auto-pull.service" >> git-auto-pull.spec
echo "" >> git-auto-pull.spec
echo "%clean" >> git-auto-pull.spec
echo 'rm -rf $RPM_BUILD_ROOT' >> git-auto-pull.spec
echo "" >> git-auto-pull.spec
echo "%post" >> git-auto-pull.spec
echo "systemctl daemon-reload || true" >> git-auto-pull.spec
echo "cp /usr/share/git-auto-pull/qsc.public.key /etc/pki/rpm-gpg/RPM-GPG-KEY-QSC-COMP66 || true" >> git-auto-pull.spec
echo "rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-QSC-COMP66 2>&1 >/dev/null || true" >> git-auto-pull.spec
echo "systemctl restart git-auto-pull || true" >> git-auto-pull.spec
echo "cp /usr/share/git-auto-pull/qsc.repo /etc/yum.repos.d/qsc.repo || true" >> git-auto-pull.spec
echo "" >> git-auto-pull.spec
echo "%changelog" >> git-auto-pull.spec
echo "* Mon Feb 09 2015 Senorsen <sen@senorsen.com> - 0.4.54" >> git-auto-pull.spec
echo "- Add support for GitHub push webhook" >> git-auto-pull.spec
echo "* Fri Feb 06 2015 Senorsen <sen@senorsen.com> - 0.1.9" >> git-auto-pull.spec
echo "- Initial version of the package" >> git-auto-pull.spec

