#!/usr/bin/env bash

echo '%sudo ALL=NOPASSWD: ALL' >> /etc/sudoers

abort() {
  exec 1>&2
  echo "$1"
  exit 1
}

[[ $USER_HOME && $USER_NAME && $USER_GID && $USER_UID && $PROJECT_DIR ]] \
  || abort "required env vars not set"

addgroup --quiet --gid ${USER_GID} ${USER_NAME}
adduser --quiet --disabled-password --home ${USER_HOME} --gid ${USER_GID} \
  --uid ${USER_UID} --gecos ${USER_NAME} ${USER_NAME}
usermod -a -G sudo ${USER_NAME}

echo -e "\033[1;35m"
cat << 'EOT'
  ______   _______ ______ __ _______     _______ _______ ______  ___ _______
 |   _  \ |   _   |   _  |  |       |   |   _   |   _   |   _  \|   |   _   |
 |.  |   \|.  |   |.  |   |_|.|   | |   |.  1   |.  1   |.  |   |.  |.  1___|
 |.  |    |.  |   |.  |   | `-|.  |-'   |.  ____|.  _   |.  |   |.  |.  |___
 |:  1    |:  1   |:  |   |   |:  |     |:  |   |:  |   |:  |   |:  |:  1   |
 |::.. . /|::.. . |::.|   |   |::.|     |::.|   |::.|:. |::.|   |::.|::.. . |
 `------' `-------`--- ---'   `---'     `---'   `--- ---`--- ---`---`-------'

EOT
echo -e "\033[0m"

cd "${PROJECT_DIR}"

su $USER_NAME
