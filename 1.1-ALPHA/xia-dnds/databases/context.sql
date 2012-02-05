--
-- See COPYRIGHTS file.
--


-- context: table for DNDS_CONTEXT object
--
-- @id              INT     DNDS_CONTEXT identifier
-- @dns_zone        STR     Name of the associated DNS zone
-- @dns_serial      INT     Serial number of the zone
-- @vhost           STR     Virtual Host
-- @addr_pool       INT     Link to DNDS_ADDR_POOL identifier
-- @desc            STR     Description
-- @age             INT     Age
--
CREATE TABLE `context` (
    `id`            SMALLINT UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `dns_zone`      VARCHAR(128) NOT NULL UNIQUE,
    `dns_serial`    INTEGER UNSIGNED NOT NULL DEFAULT 1,
    `wwwhost`       VARCHAR(128),
    `addr_pool`     INTEGER UNSIGNED NOT NULL,
    `desc`          TEXT,
    `age`           INTEGER UNSIGNED NOT NULL DEFAULT 0,

    PRIMARY KEY (`id`, `dns_zone`)
);
