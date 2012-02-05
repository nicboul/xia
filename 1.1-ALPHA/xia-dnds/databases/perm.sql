--
-- See COPYRIGHTS file.
--


-- node_perm: table for DNDS_PERM object
--
-- @id              INT     DNDS_PERM identifier
-- @name            STR     Name
-- @matrix          BLOB    Permission matrix
-- @age             INT     Age
--
CREATE TABLE `perm` (
    `id`            INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `name`          VARCHAR(64),
    `matrix`        BLOB,
    `age`           INTEGER UNSIGNED NOT NULL DEFAULT 0
);
