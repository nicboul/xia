-- This is the database structure of XIA
--
-- CONFIG_DB_NAME       Name of database

-- CONFIG_USER_XIA      Username used by XIA daemon
-- CONFIG_PASS_XIA      Password used by XIA daemon
-- CONFIG_HOST_XIA      Host running this XIA daemon

-- CONFIG_USER_WEB      Username used by WWW interfaces
-- CONFIG_PASS_WEB      Password used by WWW interfaces
-- CONFIG_HOST_WEB      Host running this WWW interface

/*
 * See COPYRIGHTS file.
 */

USE `mysql`;

--
-- We declare all services username and password here
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

REPLACE INTO `user` (`host`, `user`, `password`)
    VALUES (
        'CONFIG_HOST_WEB',
        'CONFIG_USER_WEB',
        PASSWORD('CONFIG_PASS_WEB')
);

FLUSH PRIVILEGES;

DROP DATABASE IF EXISTS `CONFIG_DB_NAME`;
CREATE DATABASE `CONFIG_DB_NAME`;

USE `CONFIG_DB_NAME`;

delimiter //

-- XIA_TOKEN(): description
--
-- @username    STR     Username in the `name@context' form
--
CREATE FUNCTION `XIA_TOKEN` (
    `username`  VARCHAR(64)
) RETURNS INTEGER DETERMINISTIC READS SQL DATA
BEGIN
    DECLARE `ret` INTEGER;

    SELECT `id` INTO `ret` FROM `tokens` WHERE
        `name` LIKE SUBSTRING_INDEX(`username`, '@', 1) AND
        `context` = SUBSTRING_INDEX(`username`, '@', -1);

    RETURN `ret`;
END;
//

delimiter ;

-- contexts: description
--
-- @id              INT     Context identifier
-- @dns_zone        STR     Name of the associated DNS zone
-- @dns_serial      INT     Serial number of the zone
-- @wwwhost         STR     Virtual Host ServerName
-- @desc            STR     Description
CREATE TABLE `contexts` (
    `id`            SMALLINT UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `dns_zone`      VARCHAR(128) NOT NULL UNIQUE,
    `dns_serial`    INTEGER DEFAULT 1 NOT NULL,
    `wwwhost`       VARCHAR(128),
    `desc`          TEXT,

    PRIMARY KEY (`id`, `dns_zone`)
);

-- contexts: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `contexts` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `contexts` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;

-- tokens: description
--
-- @id          INT     Token identifier
-- @name        STR     XIA name
-- @context     INT     Link to Context identifier
-- @status      INT     Account status (xia/session_msg.h)
-- @type        INT     Token type (xia/xiap.h)
--
CREATE TABLE `tokens` (
    `id`        INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `name`      VARCHAR(58) NOT NULL,
    `context`   SMALLINT UNSIGNED NOT NULL,
    `status`    TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `type`      TINYINT UNSIGNED NOT NULL DEFAULT 1,

    PRIMARY KEY (`context`, `name`)
);

-- tokens: privileges
--
--	READ ONLY: xiad
--	READ/WRITE: web
--
GRANT SELECT ON `tokens` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `tokens` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;

-- usr_tokens: description
--
-- @token       INT     Link to Token identifier
-- @password    STR     Web password
-- @username    STR     Web username
-- @firstname   STR     First name
-- @lastname    STR     Last name
-- @email       STR     Valid email address
-- @web_roles   STR     A place to store a serialize php array
-- @mod_ts      TIME    Last modification timestamp
--
CREATE TABLE `usr_tokens` (
    `token`     INTEGER UNSIGNED NOT NULL,
    `username`  VARCHAR(45) NOT NULL,
    `password`  BLOB NOT NULL,
    `firstname` VARCHAR(64) NOT NULL,
    `lastname`  VARCHAR(64),
    `email`     VARCHAR(256) NOT NULL,
    `web_roles` BLOB,
    `mod_ts`    TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL ON UPDATE CURRENT_TIMESTAMP,

    PRIMARY KEY (`token`, `username`)
);

-- usr_tokens: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `usr_tokens` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `usr_tokens` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;


-- res_tokens: description
--
--  This table contains informations about a ressource token.
--
--  XXX - Allow a one-to-many mapping untill all ressources can be
--        managed separately. Not possible for the moment because
--        a bridge hides the ressources behind itself.
--
-- @token       INT     Link to Token identifier
-- @hostname    STR     Hostname
-- @haddr       STR     Hardware Address
-- @addr        STR     IP Address
-- @status      INT     Status: 0 = dead, 1 = alive, 2 = unknown
-- @os_name     STR     Name of the Operating System
-- @desc        STR     A description, a note, etc.
--
CREATE TABLE `res_tokens` (
    `token`     INTEGER UNSIGNED NOT NULL,
    `hostname`  VARCHAR(64),
    `haddr`     VARCHAR(12) NOT NULL,
    `addr`      VARCHAR(15) NOT NULL,
    `status`    TINYINT UNSIGNED NOT NULL DEFAULT 2,
    `os_name`   VARCHAR(128),
    `desc`      TEXT,

    PRIMARY KEY (`token`, `haddr`)
);

-- res_tokens: privileges
--
--  READ ONLY:
--  READ/WRITE: xiad, web
--
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `res_tokens` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `res_tokens` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;


-- br_tokens: description
--
-- @token       INT     Link to Token identifier
-- @owned_by    INT     Link to Context identifier
--
CREATE TABLE `br_tokens` (
    `token`     INTEGER UNSIGNED NOT NULL UNIQUE,
    `owned_by`  INTEGER UNSIGNED NOT NULL,

    PRIMARY KEY (`token`, `owned_by`)
);

-- br_tokens: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `br_tokens` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `br_tokens` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;


-- tun_tokens: description
--
-- @token       INT     Link to Token identifier
--
CREATE TABLE `tun_tokens` (
    `token`     INTEGER UNSIGNED NOT NULL UNIQUE,

    PRIMARY KEY (`token`)
);

-- tun_tokens: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `tun_tokens` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `tun_tokens` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;


-- tokens_addr_map: description
--
-- @token   INT     Link to Token identifier
-- @addr    STR     Associated IP address
--
CREATE TABLE `tokens_addr_map` (
    `token` INTEGER UNSIGNED NOT NULL UNIQUE,
    `addr`  VARCHAR(15) NOT NULL UNIQUE,

    PRIMARY KEY (`token`, `addr`)
);

-- tokens_addr_map: privileges
--
--  READ ONLY: web
--  READ/WRITE: xiad
--
GRANT SELECT ON `tokens_addr_map` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `tokens_addr_map` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;


-- journal: description
--
-- Not in use yet.
--
-- NOTE: The in and out statistics are switched to reflect
--       the other side of the tunnel.
--
-- @token       INT   Link to Token identifier
-- @up_ts       TIME  Interface is up, timestamp
-- @down_ts     TIME  Interface is down, timestamp
-- @duration    INT   Connection duration time
-- @bytes_in    INT   Number of bytes sent to client
-- @bytes_out   INT   Number of bytes received from client
-- @pckts_in    INT   Number of packets sent to client
-- @pckts_out   INT   Number of packets received from client
-- @remote_ip   INT   Client IP where the connection came from
--
CREATE TABLE `journal` (
    `token`      INTEGER UNSIGNED NOT NULL,
    `up_ts`     TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
    `down_ts`   TIMESTAMP,
    `duration`  INTEGER UNSIGNED,
    `bytes_in`  BIGINT  UNSIGNED,
    `bytes_out` BIGINT  UNSIGNED,
    `pckts_in`  BIGINT  UNSIGNED,
    `pckts_out` BIGINT  UNSIGNED,
    `remote_ip` INTEGER UNSIGNED NOT NULL,

    PRIMARY KEY (`token`, `up_ts`)
);


-- acl_groups: description
--
-- @id          INT     ACL Group identifier
-- @context     INT     Link to Context identifier
-- @name        STR     Name of this group
-- @desc        STR     Description
--
CREATE TABLE `acl_groups` (
    `id`        INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `context`   SMALLINT UNSIGNED NOT NULL,
    `name`      VARCHAR(128) NOT NULL,
    `desc`      TEXT,

    PRIMARY KEY(`context`, `name`)
);

-- acl_groups: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `acl_groups` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `acl_groups` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;


-- acl_groups_map: description
--
-- @acl_group   INT     Link to ACL Group identifier
-- @token       INT     Link to Token identifier
--
CREATE TABLE `acl_groups_map` (
    `acl_group` INTEGER UNSIGNED NOT NULL,
    `token`     INTEGER UNSIGNED NOT NULL,

    PRIMARY KEY(`acl_group`, `token`)
);

-- acl_groups_map: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `acl_groups_map` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `acl_groups_map` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;


-- acl_sets: description
--
-- @id              INT     ACL Set identifier
-- @context         INT     Link to Context identifier
-- @desc            STR     Description
--
CREATE TABLE `acl_sets` (
    `id`            INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `context`       SMALLINT UNSIGNED NOT NULL,
    `desc`          TEXT,
    
    PRIMARY KEY (`id`)
);

-- acl_sets: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `acl_sets` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `acl_sets` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;


-- acl_sets_usr_map: description
--
-- @acl_set     INT     Link to ACL Set identifier
-- @acl_group   INT     Link to ACL Group identifier
--
CREATE TABLE `acl_sets_usr_map` (
    `acl_set`   INTEGER UNSIGNED NOT NULL,
    `acl_group` INTEGER UNSIGNED NOT NULL,

    PRIMARY KEY (`acl_set`, `acl_group`)
);

-- acl_sets_usr_map: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `acl_sets_usr_map` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `acl_sets_usr_map` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;


-- acl_sets_res_map: description
--
-- @acl_set     INT     Link to ACL Set identifier
-- @token       INT     Link to Token identifier
--
CREATE TABLE `acl_sets_res_map` (
    `acl_set`   INTEGER UNSIGNED NOT NULL,
    `token`     INTEGER UNSIGNED NOT NULL,

    PRIMARY KEY (`acl_set`, `token`)
);

-- acl_sets_res_map: privileges
--
--  READ ONLY: xiad
--  READ/WRITE: web
--
GRANT SELECT ON `acl_sets_res_map` TO `CONFIG_USER_XIA`@`CONFIG_HOST_XIA`;
GRANT SELECT, INSERT, UPDATE, DELETE
        ON `acl_sets_res_map` TO `CONFIG_USER_WEB`@`CONFIG_HOST_WEB`;
