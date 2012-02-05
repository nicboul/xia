--
-- See COPYRIGHTS file.
--


-- peer: table for DNDS_PEER object
--
-- @id          INT     DNDS_PEER identifier
-- @context     INT     Link to DNDS_CONTEXT identifier
-- @name        STR     Name
-- @addr        INT     IP address in network byte order
-- @flag        INT     Flags
-- @age         INT     Age
--
CREATE TABLE `peer` (
    `id`            INTEGER UNSIGNED NOT NULL UNIQUE AUTO_INCREMENT,
    `context`       SMALLINT UNSIGNED NOT NULL,
    `name`          VARCHAR(64) NOT NULL,
    `addr`          INTEGER UNSIGNED,
    `flag`          TINYINT UNSIGNED,
    `age`           INTEGER UNSIGNED NOT NULL DEFAULT 0,

    PRIMARY KEY (`id`)
);

-- peer_user_map
--
-- @peer        INT     Link to DNDS_PEER identifier
-- @user        INT     Link to DNDS_USER identifier
--
CREATE TABLE `peer_user_map` (
    `id`            INTEGER UNSIGNED NOT NULL,
    `user`          INTEGER UNSIGNED NOT NULL 
);
