FROM ubuntu:14.04
MAINTAINER micha@adzerk.com

RUN apt-get update

RUN apt-get install -y build-essential python curl

RUN curl https://nodejs.org/dist/v5.12.0/node-v5.12.0-linux-x64.tar.gz \
      | (cd /tmp && tar xzf - && cd node-v5.12.0-linux-x64 && tar cf - .) \
      | (cd /usr/local && tar xf -) \
      && npm install -g node-gyp \
      && npm install -g mocha \
      && npm install -g coffee-script \
      && ln -s /usr/local/lib/node_modules /usr/local/lib/node

ADD startup.sh /startup.sh
RUN chmod 755 /startup.sh

RUN locale-gen "en_US.UTF-8" && dpkg-reconfigure locales
