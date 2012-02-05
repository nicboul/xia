#!/bin/sh
#
# Create database script
#

DB_HOST=localhost
DB_USERNAME=root

CONFIG_DB_NAME=xia

CONFIG_USER_XIA=xia_user
CONFIG_PASS_XIA=xia_pass
CONFIG_HOST_XIA=localhost

cat init.sql acl.sql acl_group.sql addr_pool.sql context.sql              \
    host.sql node.sql peer.sql perm.sql user.sql                          \
              | sed s/CONFIG_DB_NAME/$CONFIG_DB_NAME/g                    \
              | sed s/CONFIG_USER_XIA/$CONFIG_USER_XIA/g                  \
              | sed s/CONFIG_PASS_XIA/$CONFIG_PASS_XIA/g                  \
              | sed s/CONFIG_HOST_XIA/$CONFIG_HOST_XIA/g                  \
              | mysql -v -v -v -h $DB_HOST -u $DB_USERNAME -p
