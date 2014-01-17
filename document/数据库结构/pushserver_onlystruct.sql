/*
SQLyog Community Edition- MySQL GUI v6.56
MySQL - 5.0.95 : Database - pushserver
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

CREATE DATABASE /*!32312 IF NOT EXISTS*/`pushserver` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `pushserver`;

/*Table structure for table `client_info` */

DROP TABLE IF EXISTS `client_info`;

CREATE TABLE `client_info` (
  `tokenid` varchar(256) NOT NULL,
  `appid` varchar(256) default NULL,
  `appver` varchar(256) default NULL,
  `model` varchar(256) default NULL,
  `system` varchar(256) default NULL,
  PRIMARY KEY  (`tokenid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `client_online_record` */

DROP TABLE IF EXISTS `client_online_record`;

CREATE TABLE `client_online_record` (
  `id` int(11) NOT NULL auto_increment,
  `tokenid` varchar(256) default NULL,
  `ip` varchar(32) default NULL,
  `port` int(11) default NULL,
  `start` datetime default NULL,
  `end` datetime default NULL,
  `offlinemsg` int(11) default NULL,
  `onlinemsg` int(11) default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=428827 DEFAULT CHARSET=utf8;

/*Table structure for table `ip_location` */

DROP TABLE IF EXISTS `ip_location`;

CREATE TABLE `ip_location` (
  `start_ip` varchar(32) NOT NULL,
  `end_ip` varchar(32) NOT NULL,
  `area` varchar(256) default NULL,
  `location` varchar(256) default NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `push_identification` */

DROP TABLE IF EXISTS `push_identification`;

CREATE TABLE `push_identification` (
  `appid` varchar(256) default NULL,
  `appkey` varchar(256) default NULL,
  `desc` varchar(256) default NULL,
  `applastver` varchar(32) default NULL,
  `apppkgurl` varchar(1024) character set ascii default NULL,
  `appverdesc` varchar(256) default NULL,
  `updatestatus` int(11) default '0',
  `testtokenid` varchar(1024) default NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `push_message` */

DROP TABLE IF EXISTS `push_message`;

CREATE TABLE `push_message` (
  `tokenid` varchar(256) default NULL,
  `message` varchar(2048) default NULL,
  `id` int(11) NOT NULL auto_increment,
  `recvtime` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=13313 DEFAULT CHARSET=utf8;

/*Table structure for table `send_push_message` */

DROP TABLE IF EXISTS `send_push_message`;

CREATE TABLE `send_push_message` (
  `tokenid` varchar(256) default NULL,
  `message` varchar(2048) default NULL,
  `recvtime` timestamp NOT NULL default '0000-00-00 00:00:00',
  `sendtime` timestamp NOT NULL default CURRENT_TIMESTAMP
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/* Trigger structure for table `push_message` */

DELIMITER $$

/*!50003 DROP TRIGGER*//*!50032 IF EXISTS */ /*!50003 `t_delete_pushmessage` */$$

/*!50003 CREATE */  /*!50003 TRIGGER `t_delete_pushmessage` AFTER DELETE ON `push_message` FOR EACH ROW BEGIN
	INSERT INTO send_push_message (tokenid,message,recvtime) VALUES (OLD.tokenid, OLD.message, OLD.recvtime);
    END */$$


DELIMITER ;

/* Procedure structure for procedure `s_client_fristonline_time` */

/*!50003 DROP PROCEDURE IF EXISTS  `s_client_fristonline_time` */;

DELIMITER $$

/*!50003 CREATE DEFINER=``@`` PROCEDURE `s_client_fristonline_time`(IN theAppId VARCHAR(256), IN theTimeLimit VARCHAR(64))
BEGIN
	DECLARE strsql varchar(2048);
	SET strsql = CONCAT('(
		SELECT `v_client_fristonline_time`.`tokenid`,`client_start`,`push_identification`.`desc`,`client_info`.`model`,`client_info`.`appver` 
		FROM ((`v_client_fristonline_time` LEFT JOIN `client_info` ON((`v_client_fristonline_time`.`tokenid` = `client_info`.`tokenid`))) LEFT JOIN `push_identification` ON((`client_info`.`appid` = `push_identification`.`appid`))) 
		WHERE `client_info`.`appid`=\'', theAppId, '\' AND UNIX_TIMESTAMP(`client_start`) >= UNIX_TIMESTAMP(\'', theTimeLimit, '\')
		ORDER BY `client_start` DESC)');
	SET @sql1 = strsql;     
	PREPARE stmt_p FROM @sql1;     
	EXECUTE stmt_p; 
	DEALLOCATE PREPARE stmt_p;
    END */$$
DELIMITER ;

/* Procedure structure for procedure `s_client_lastonline_time` */

/*!50003 DROP PROCEDURE IF EXISTS  `s_client_lastonline_time` */;

DELIMITER $$

/*!50003 CREATE DEFINER=``@`` PROCEDURE `s_client_lastonline_time`(IN theAppId VARCHAR(256), IN theTimeLimit VARCHAR(64))
BEGIN
	DECLARE strsql varchar(2048);
	SET strsql = CONCAT('(
		SELECT `v_client_lastonline_time`.`tokenid`,`client_end`,`push_identification`.`desc`,`client_info`.`model`,`client_info`.`appver` 
		FROM ((`v_client_lastonline_time` LEFT JOIN `client_info` ON((`v_client_lastonline_time`.`tokenid` = `client_info`.`tokenid`))) LEFT JOIN `push_identification` ON((`client_info`.`appid` = `push_identification`.`appid`))) 
		WHERE `client_info`.`appid`=\'', theAppId, '\' AND UNIX_TIMESTAMP(`client_end`) >= UNIX_TIMESTAMP(\'', theTimeLimit, '\')
		ORDER BY `client_end` DESC)');
	SET @sql1 = strsql;     
	PREPARE stmt_p FROM @sql1;     
	EXECUTE stmt_p; 
	DEALLOCATE PREPARE stmt_p;
    END */$$
DELIMITER ;

/* Procedure structure for procedure `s_insert_pushmsg` */

/*!50003 DROP PROCEDURE IF EXISTS  `s_insert_pushmsg` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`%` PROCEDURE `s_insert_pushmsg`(IN theAppId VARCHAR(256), IN theBody VARCHAR(1024))
BEGIN
	DECLARE strsql varchar(2048);
	SET strsql = CONCAT('
	INSERT INTO push_message (push_message.tokenid,push_message.message)
	  (SELECT client_info.tokenid,\'{"aps":{"alert":"', theBody, '","bodyID":1,"sound":"default"},"etype":1}\' 
	   FROM client_info 
	   WHERE client_info.appid=\'', theAppId,'\')');
	SET @sql1 = strsql;     
	PREPARE stmt_p FROM @sql1;     
	EXECUTE stmt_p; 
	DEALLOCATE PREPARE stmt_p;
    END */$$
DELIMITER ;

/* Procedure structure for procedure `s_online_record` */

/*!50003 DROP PROCEDURE IF EXISTS  `s_online_record` */;

DELIMITER $$

/*!50003 CREATE DEFINER=``@`` PROCEDURE `s_online_record`(IN theTokenid VARCHAR(256), IN topCords INT)
BEGIN
	DECLARE strsql varchar(2048);
	SET strsql = CONCAT('
	SELECT 
	    `client_online_record`.`tokenid` AS `tokenid`
	    ,SEC_TO_TIME(unix_timestamp(`client_online_record`.`end`) - unix_timestamp(`client_online_record`.`start`)) AS `diff`
	    ,`client_info`.`model` AS `model`
	    ,`push_identification`.`desc` AS `appdesc`
	    ,`client_info`.`appver` AS `appver`
	    ,`client_online_record`.`start` AS `start`
	    ,`client_online_record`.`end` AS `end`
	    ,`client_online_record`.`offlinemsg` AS `offmsg`
	    ,`client_online_record`.`onlinemsg` AS `onmsg`
	    ,`client_online_record`.`ip` AS `ip` 
	FROM 
	    `client_online_record` LEFT JOIN `client_info` 
	       ON `client_online_record`.`tokenid`=`client_info`.`tokenid`
	    LEFT JOIN `push_identification` ON `client_info`.`appid`=`push_identification`.`appid`
	WHERE 
	    (`client_online_record`.`tokenid`=\'', theTokenid,'\')
	ORDER BY 
	    `client_online_record`.`end` DESC
	LIMIT '
	    ,topCords);
	
	SET @sql1 = strsql;     
	PREPARE stmt_p FROM @sql1;     
	EXECUTE stmt_p; 
	DEALLOCATE PREPARE stmt_p;
    END */$$
DELIMITER ;

/*Table structure for table `v_client_fristonline_time` */

DROP TABLE IF EXISTS `v_client_fristonline_time`;

/*!50001 DROP VIEW IF EXISTS `v_client_fristonline_time` */;
/*!50001 DROP TABLE IF EXISTS `v_client_fristonline_time` */;

/*!50001 CREATE TABLE `v_client_fristonline_time` (
  `tokenid` varchar(256) default NULL,
  `client_start` datetime default NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8 */;

/*Table structure for table `v_client_info` */

DROP TABLE IF EXISTS `v_client_info`;

/*!50001 DROP VIEW IF EXISTS `v_client_info` */;
/*!50001 DROP TABLE IF EXISTS `v_client_info` */;

/*!50001 CREATE TABLE `v_client_info` (
  `tokenid` varchar(256) NOT NULL,
  `desc` varchar(256) default NULL,
  `appver` varchar(256) default NULL,
  `model` varchar(256) default NULL,
  `system` varchar(256) default NULL,
  `appid` varchar(256) default NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8 */;

/*Table structure for table `v_client_lastonline_time` */

DROP TABLE IF EXISTS `v_client_lastonline_time`;

/*!50001 DROP VIEW IF EXISTS `v_client_lastonline_time` */;
/*!50001 DROP TABLE IF EXISTS `v_client_lastonline_time` */;

/*!50001 CREATE TABLE `v_client_lastonline_time` (
  `tokenid` varchar(256) default NULL,
  `client_end` datetime default NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8 */;

/*Table structure for table `v_client_online_record` */

DROP TABLE IF EXISTS `v_client_online_record`;

/*!50001 DROP VIEW IF EXISTS `v_client_online_record` */;
/*!50001 DROP TABLE IF EXISTS `v_client_online_record` */;

/*!50001 CREATE TABLE `v_client_online_record` (
  `tokenid` varchar(256) default NULL,
  `diff` time default NULL,
  `model` varchar(256) default NULL,
  `appdesc` varchar(256) default NULL,
  `appver` varchar(256) default NULL,
  `start` datetime default NULL,
  `end` datetime default NULL,
  `offmsg` int(11) default NULL,
  `onmsg` int(11) default NULL,
  `ip` varchar(32) default NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8 */;

/*Table structure for table `v_clientinfo_count_by_app` */

DROP TABLE IF EXISTS `v_clientinfo_count_by_app`;

/*!50001 DROP VIEW IF EXISTS `v_clientinfo_count_by_app` */;
/*!50001 DROP TABLE IF EXISTS `v_clientinfo_count_by_app` */;

/*!50001 CREATE TABLE `v_clientinfo_count_by_app` (
  `totle` bigint(21) NOT NULL default '0',
  `desc` varchar(256) default NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8 */;

/*View structure for view v_client_fristonline_time */

/*!50001 DROP TABLE IF EXISTS `v_client_fristonline_time` */;
/*!50001 DROP VIEW IF EXISTS `v_client_fristonline_time` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=``@`` SQL SECURITY DEFINER VIEW `v_client_fristonline_time` AS select `client_online_record`.`tokenid` AS `tokenid`,min(`client_online_record`.`start`) AS `client_start` from `client_online_record` group by `client_online_record`.`tokenid` order by min(`client_online_record`.`start`) desc */;

/*View structure for view v_client_info */

/*!50001 DROP TABLE IF EXISTS `v_client_info` */;
/*!50001 DROP VIEW IF EXISTS `v_client_info` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=``@`` SQL SECURITY DEFINER VIEW `v_client_info` AS (select `ci`.`tokenid` AS `tokenid`,`pi`.`desc` AS `desc`,`ci`.`appver` AS `appver`,`ci`.`model` AS `model`,`ci`.`system` AS `system`,`ci`.`appid` AS `appid` from (`client_info` `ci` join `push_identification` `pi`) where (`ci`.`appid` = `pi`.`appid`) order by `pi`.`desc`) */;

/*View structure for view v_client_lastonline_time */

/*!50001 DROP TABLE IF EXISTS `v_client_lastonline_time` */;
/*!50001 DROP VIEW IF EXISTS `v_client_lastonline_time` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=``@`` SQL SECURITY DEFINER VIEW `v_client_lastonline_time` AS select `client_online_record`.`tokenid` AS `tokenid`,max(`client_online_record`.`end`) AS `client_end` from `client_online_record` group by `client_online_record`.`tokenid` order by max(`client_online_record`.`end`) desc */;

/*View structure for view v_client_online_record */

/*!50001 DROP TABLE IF EXISTS `v_client_online_record` */;
/*!50001 DROP VIEW IF EXISTS `v_client_online_record` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=``@`` SQL SECURITY DEFINER VIEW `v_client_online_record` AS (select `client_online_record`.`tokenid` AS `tokenid`,sec_to_time((unix_timestamp(`client_online_record`.`end`) - unix_timestamp(`client_online_record`.`start`))) AS `diff`,`client_info`.`model` AS `model`,`push_identification`.`desc` AS `appdesc`,`client_info`.`appver` AS `appver`,`client_online_record`.`start` AS `start`,`client_online_record`.`end` AS `end`,`client_online_record`.`offlinemsg` AS `offmsg`,`client_online_record`.`onlinemsg` AS `onmsg`,`client_online_record`.`ip` AS `ip` from ((`client_online_record` left join `client_info` on((`client_online_record`.`tokenid` = `client_info`.`tokenid`))) left join `push_identification` on((`client_info`.`appid` = `push_identification`.`appid`))) order by sec_to_time((unix_timestamp(`client_online_record`.`end`) - unix_timestamp(`client_online_record`.`start`))) desc limit 50) */;

/*View structure for view v_clientinfo_count_by_app */

/*!50001 DROP TABLE IF EXISTS `v_clientinfo_count_by_app` */;
/*!50001 DROP VIEW IF EXISTS `v_clientinfo_count_by_app` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=``@`` SQL SECURITY DEFINER VIEW `v_clientinfo_count_by_app` AS select count(0) AS `totle`,`v_client_info`.`desc` AS `desc` from `v_client_info` group by `v_client_info`.`appid` order by count(0) desc */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
