--
-- See COPYRIGHTS file.
--


-- acl_group: table for DNDS_ACL_GROUP object
--
-- @id          INT     DNDS_ACL_GROUP identifier
-- @context     INT     Link to DNDS_CONTEXT identifier
-- @name        STR     Name of this group
-- @desc        STR     Description
-- @age         INT     Age
--
CREATE TABLE `acl_group` (
    `id`        INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `context`   SMALLINT UNSIGNED NOT NULL,
    `name`      VARCHAR(128) NOT NULL,
    `desc`      TEXT,
    `age`       INTEGER UNSIGNED NOT NULL DEFAULT 0,

    PRIMARY KEY(`id`)
);

-- acl_group_map
--
-- @acl_group   INT     Link to DNDS_ACL_GROUP identifier
-- @host        INT     Link to DNDS_HOST identifier
--
CREATE TABLE `acl_group_map` (
    `acl_group` INTEGER UNSIGNED NOT NULL,
    `host`      INTEGER UNSIGNED NOT NULL
);
