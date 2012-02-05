--
-- See COPYRIGHTS file.
--


-- Initialize database
--
-- CONFIG_DB_NAME       Name of database

-- CONFIG_USER_DNDS     Username used by DNDS daemon
-- CONFIG_PASS_DNDS     Password used by DNDS daemon
-- CONFIG_HOST_DNDS     Host running this DNDS daemon

USE `mysql`;

--
-- Create user
--
REPLACE INTO `user` (`host`, `user`, `password`)
    VALUES (
        'CONFIG_HOST_XIA',
        'CONFIG_USER_XIA',
        PASSWORD('CONFIG_PASS_XIA')
);

REPLACE INTO `db` (`host`, `db`, `user`, `execute_priv`)
    VALUES (
        'CONFIG_HOST_XIA',
        'CONFIG_DB_NAME',
        'CONFIG_USER_XIA',
        'Y'
);

FLUSH PRIVILEGES;

--
-- Create database
--
DROP DATABASE IF EXISTS `CONFIG_DB_NAME`;
CREATE DATABASE `CONFIG_DB_NAME`;

USE `CONFIG_DB_NAME`;
