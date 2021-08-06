#!/bin/bash
set -euo pipefail

INSTALLER_SHA256=f5ec72f7abcf53d55f39c0f7f87a9fc1cda6f27f3886268fe15553059be4f097
INSTALLER_URL='https://portswigger.net/burp/releases/download?product=community&version=2021.8&type=Linux'
INSTALLER=burpsuite_community_linux_v2021_8.sh
INSTALL_DIR=/opt/BurpSuiteCommunity

if [ -d $INSTALL_DIR ]; then
    echo "already installed"
    exit 0
fi

echo "downloading installer"
curl -x 127.0.0.1:8082 -sfL -o $INSTALLER $INSTALLER_URL

echo "verifying installer integrity"
echo "$INSTALLER_SHA256  $INSTALLER" | sha256sum -c

echo "running installer"
cat <<EOF >response.varfile
# install4j response file for Burp Suite Community Edition 2021.8
sys.adminRights\$Boolean=false
sys.installationDir=$INSTALL_DIR
sys.languageId=en
sys.programGroupDisabled\$Boolean=false
sys.symlinkDir=/usr/local/bin
EOF
chmod +x $INSTALLER
./$INSTALLER -q -varfile response.varfile

echo "cleaning up"
rm -rf $INSTALLER response.varfile

echo "moving desktop file"
cd $INSTALL_DIR
mv $(readlink "Burp Suite Community Edition.desktop") /usr/share/applications/
rm -f "Burp Suite Community Edition.desktop"

echo "done"
