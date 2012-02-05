--
-- See COPYRIGHTS file.
--


-- node: table for DNDS_NODE object
--
-- @id          INT     DNDS_NODE identifier
-- @name        STR     Name
-- @type        INT     Type
-- @addr        INT     IP address in network byte order
-- @perm        INT     Link to DNDS_PERM identifier
-- @flag        INT     Flags
-- @age         INT     Age
--
CREATE TABLE `node` (
    `id`            INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `name`          VARCHAR(64) NOT NULL UNIQUE,
    `type`          TINYINT UNSIGNED,
    `addr`          INTEGER UNSIGNED,
    `perm`          INTEGER UNSIGNED,
    `flag`          TINYINT UNSIGNED,
    `age`           INTEGER UNSIGNED NOT NULL DEFAULT 0,

    PRIMARY KEY (`id`)
);
