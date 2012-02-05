#!/bin/sh
#
# This scripts imports and populates the core database into MySQL
#

DB_HOST=localhost
DB_USERNAME=root

CONFIG_DB_NAME=xia

CONFIG_USER_XIA=xia_user
CONFIG_PASS_XIA=xia_pass
CONFIG_HOST_XIA=localhost

CONFIG_USER_WEB=web_user
CONFIG_PASS_WEB=web_pass
CONFIG_HOST_WEB=localhost

# This is a variable substitution kludge!

cat core.sql  | sed s/CONFIG_DB_NAME/$CONFIG_DB_NAME/g                    \
              | sed s/CONFIG_USER_XIA/$CONFIG_USER_XIA/g                  \
              | sed s/CONFIG_PASS_XIA/$CONFIG_PASS_XIA/g                  \
              | sed s/CONFIG_HOST_XIA/$CONFIG_HOST_XIA/g                  \
              | sed s/CONFIG_USER_WEB/$CONFIG_USER_WEB/g                  \
              | sed s/CONFIG_PASS_WEB/$CONFIG_PASS_WEB/g                  \
              | sed s/CONFIG_HOST_WEB/$CONFIG_HOST_WEB/g                  \
              | mysql -v -v -v -h $DB_HOST -u $DB_USERNAME -p
