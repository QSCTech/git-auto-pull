#!/bin/sh

echo "Package: git-auto-pull" > control
echo "Version: $2-$3" >> control
echo "Section: development" >> control
echo "Priority: optional" >> control
echo "Architecture: $4" >> control
echo "Depends: git, ca-certificates" >> control
echo "Installed-Size: " `expr \`du -b $1 | cut -f -1\` / 1024` >> control
echo "Maintainer: Senorsen <sen@senorsen.com>" >> control
echo "Description: GitLab and GitHub Webhook auto pull service. Qiu Shi Chao Website. " >> control

