--
-- See COPYRIGHTS file.
--


-- user: table for DNDS_USER object
--
-- @id          INT     DNDS_USER identifier
-- @context     INT     Link to DNDS_CONTEXT identifier
-- @name        STR     Name
-- @password    STR     Password
-- @firstname   STR     First name
-- @lastname    STR     Last name
-- @email       STR     Valid email address
-- @role        INT     Role
-- @flag        INT     Flags
-- @age         INT     Age
--
CREATE TABLE `user` (
    `id`        INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `context`   SMALLINT UNSIGNED NOT NULL,
    `name`      VARCHAR(45) NOT NULL,
    `password`  VARCHAR(32) NOT NULL,
    `firstname` VARCHAR(64),
    `lastname`  VARCHAR(64),
    `email`     VARCHAR(256) NOT NULL,
    `role`      TINYINT,
    `flag`      TINYINT,
    `age`       INTEGER UNSIGNED NOT NULL DEFAULT 0,

    PRIMARY KEY (`id`)
);
