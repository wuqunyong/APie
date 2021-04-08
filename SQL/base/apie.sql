/*
Navicat MySQL Data Transfer

Source Server         : localhost
Source Server Version : 50720
Source Host           : localhost:3306
Source Database       : apie

Target Server Type    : MYSQL
Target Server Version : 50720
File Encoding         : 65001

Date: 2021-04-08 09:38:37
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for role_base
-- ----------------------------
DROP TABLE IF EXISTS `role_base`;
CREATE TABLE `role_base` (
  `user_id` bigint(20) unsigned NOT NULL,
  `game_id` bigint(20) unsigned NOT NULL DEFAULT '0',
  `level` int(4) unsigned NOT NULL DEFAULT '0' COMMENT '账号等级',
  `register_time` bigint(20) NOT NULL DEFAULT '0',
  `login_time` bigint(20) NOT NULL,
  `offline_time` bigint(20) NOT NULL,
  `name` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Procedure structure for sp_select_userid_info
-- ----------------------------
DROP PROCEDURE IF EXISTS `sp_select_userid_info`;
DELIMITER ;;
CREATE DEFINER=`root`@`localhost` PROCEDURE `sp_select_userid_info`(IN `begin_id` integer,IN `batch_count` integer)
BEGIN
		select user_id,user_id
    from role_base
    where user_id>begin_id
		limit batch_count;
END
;;
DELIMITER ;
