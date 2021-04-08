/*
Navicat MySQL Data Transfer

Source Server         : localhost
Source Server Version : 50720
Source Host           : localhost:3306
Source Database       : apie_account

Target Server Type    : MYSQL
Target Server Version : 50720
File Encoding         : 65001

Date: 2021-04-08 09:38:48
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for account
-- ----------------------------
DROP TABLE IF EXISTS `account`;
CREATE TABLE `account` (
  `account_id` bigint(20) unsigned NOT NULL,
  `db_id` int(11) unsigned NOT NULL,
  `register_time` bigint(20) NOT NULL,
  `modified_time` bigint(20) NOT NULL,
  PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
