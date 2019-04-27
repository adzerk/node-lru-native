FROM node:11-alpine

# Add toolchain for compiling native modules
RUN apk add --no-cache make gcc g++ python

ARG DIR=/build
ADD . ${DIR}
WORKDIR ${DIR}

# Fetch Dependencies
RUN npm install --production --silent

# Build
RUN npm run build
