--
-- See COPYRIGHTS file.
--


-- addr_pool: table for DNDS_ADDR_POOL object
--
-- NOTE: addresses are in network byte order
--
-- @id          INT     DNDS_ADDR_POOL identifier
-- @local       INT     IP address given to local tunnel interface
-- @begin       INT     Pool begins at this IP address
-- @end         INT     Pool ends at this IP address
-- @netmask     INT     Mask of this network
-- @desc        STR     A brief description of this pool
-- @age         INT     Age
--
CREATE TABLE `addr_pool` (
    `id`        INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `local`     INTEGER UNSIGNED NOT NULL,
    `begin`     INTEGER UNSIGNED NOT NULL,
    `end`       INTEGER UNSIGNED NOT NULL,
    `netmask`   INTEGER UNSIGNED NOT NULL,
    `desc`      TEXT,
    `age`       INTEGER UNSIGNED NOT NULL DEFAULT 0,

    PRIMARY KEY (`id`)
);
