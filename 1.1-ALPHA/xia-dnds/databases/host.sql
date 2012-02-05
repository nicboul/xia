--
-- See COPYRIGHTS file.
--


-- host: table for DNDS_HOST object
--
-- @id          INT     DNDS_HOST identifier
-- @peer        INT     Link to DNDS_PEER identifier
-- @name        STR     Hostname
-- @haddr       STR     Hardware Address
-- @addr        INT     IP address in network byte order
-- @flag        INT     Flags
-- @age         INT     Age
--
CREATE TABLE `host` (
    `id`        INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `peer`      INTEGER UNSIGNED NOT NULL,
    `name`      VARCHAR(64),
    `haddr`     VARCHAR(12),
    `addr`      INTEGER UNSIGNED,
    `flag`      TINYINT UNSIGNED,
    `age`       INTEGER UNSIGNED NOT NULL DEFAULT 0,

    PRIMARY KEY (`id`)
);
