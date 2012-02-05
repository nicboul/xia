--
-- See COPYRIGHTS file.
--


-- acl: table for DNDS_ACL object
--
-- @id              INT     DNDS_ACL identifier
-- @context         INT     Link to DNDS_CONTEXT identifier
-- @desc            STR     Description
-- @age             INT     Age
--
CREATE TABLE `acl` (
    `id`            INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `context`       SMALLINT UNSIGNED NOT NULL,
    `desc`          TEXT,
    `age`           INTEGER UNSIGNED NOT NULL DEFAULT 0,
    
    PRIMARY KEY (`id`)
);

-- acl_src_map
--
-- @acl         INT     Link to DNDS_ACL identifier
-- @acl_group   INT     Link to DNDS_ACL_GROUP identifier
-- @host        INT     Link to DNDS_HOST identifier
--
CREATE TABLE `acl_src_map` (
    `acl`       INTEGER UNSIGNED NOT NULL,
    `acl_group` INTEGER UNSIGNED,
    `host`      INTEGER UNSIGNED
);

-- acl_dst_map
--
-- @acl         INT     Link to DNDS_ACL identifier
-- @acl_group   INT     Link to DNDS_ACL_GROUP identifier
-- @host        INT     Link to DNDS_HOST identifier
--
CREATE TABLE `acl_dst_map` (
    `acl`       INTEGER UNSIGNED NOT NULL,
    `acl_group` INTEGER UNSIGNED,
    `host`      INTEGER UNSIGNED
);
