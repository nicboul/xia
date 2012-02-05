-- MySQL dump 10.11
--
-- Host: localhost    Database: xia
-- ------------------------------------------------------
-- Server version	5.0.51a-24

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Dumping data for table `acl_groups`
--

LOCK TABLES `acl_groups` WRITE;
/*!40000 ALTER TABLE `acl_groups` DISABLE KEYS */;
INSERT INTO `acl_groups` VALUES (1,1,'acl-1','Group for ACL set 1'),(2,1,'XIA Hackers','Members of this group are granted network access (TCP/IP) to the SVN/Trac developement server'),(3,2,'DVR Caisse Desjardins Ste-Martine','Access au systeme de camera de la caisse Ste-Martine'),(4,3,'RÃ©seau du bureau','Les membres de  ce groupe ont accÃ¨s au serveur du bureau.');
/*!40000 ALTER TABLE `acl_groups` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `acl_groups_map`
--

LOCK TABLES `acl_groups_map` WRITE;
/*!40000 ALTER TABLE `acl_groups_map` DISABLE KEYS */;
INSERT INTO `acl_groups_map` VALUES (2,3),(2,6),(3,2),(3,5),(4,11);
/*!40000 ALTER TABLE `acl_groups_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `acl_sets`
--

LOCK TABLES `acl_sets` WRITE;
/*!40000 ALTER TABLE `acl_sets` DISABLE KEYS */;
INSERT INTO `acl_sets` VALUES (4,3,'SmakDesign Company'),(2,1,'SVN/Trac Developement server'),(3,2,'Liste de controle d\'access pour access point desjardins');
/*!40000 ALTER TABLE `acl_sets` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `acl_sets_res_map`
--

LOCK TABLES `acl_sets_res_map` WRITE;
/*!40000 ALTER TABLE `acl_sets_res_map` DISABLE KEYS */;
INSERT INTO `acl_sets_res_map` VALUES (2,7),(3,2),(4,9);
/*!40000 ALTER TABLE `acl_sets_res_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `acl_sets_usr_map`
--

LOCK TABLES `acl_sets_usr_map` WRITE;
/*!40000 ALTER TABLE `acl_sets_usr_map` DISABLE KEYS */;
INSERT INTO `acl_sets_usr_map` VALUES (1,1),(2,2),(3,3),(4,4);
/*!40000 ALTER TABLE `acl_sets_usr_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `br_tokens`
--

LOCK TABLES `br_tokens` WRITE;
/*!40000 ALTER TABLE `br_tokens` DISABLE KEYS */;
INSERT INTO `br_tokens` VALUES (1,2),(4,2),(8,3),(10,3);
/*!40000 ALTER TABLE `br_tokens` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `contexts`
--

LOCK TABLES `contexts` WRITE;
/*!40000 ALTER TABLE `contexts` DISABLE KEYS */;
INSERT INTO `contexts` VALUES (1,'xia.mind4networks.com',1,'intranet.mind4networks.com','Mind4Network Technologies INC.'),(2,'gls-intranet.mind4networks.com',1,'gls.mind4networks.com','Groupe Laplante Securite INC.'),(3,'smakdesign.mind4networks.com',1,'elis.mind4networks.com','Elis');
/*!40000 ALTER TABLE `contexts` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `stats`
--

LOCK TABLES `stats` WRITE;
/*!40000 ALTER TABLE `stats` DISABLE KEYS */;
/*!40000 ALTER TABLE `stats` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `tokens`
--

LOCK TABLES `tokens` WRITE;
/*!40000 ALTER TABLE `tokens` DISABLE KEYS */;
INSERT INTO `tokens` VALUES (1,'000024caa468',0,1,4),(2,'000024caa468',2,1,2),(3,'saj',1,1,1),(4,'000024cacb20',0,1,3),(5,'000024cacb20',2,1,1),(6,'nib',1,1,1),(7,'svn.mynodes.net',1,1,2),(8,'000024cb0a08',0,1,4),(9,'000024cb0a08',3,1,2),(10,'000024cb0a04',0,1,3),(11,'000024cb0a04',3,1,1);
/*!40000 ALTER TABLE `tokens` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `tokens_addr_map`
--

LOCK TABLES `tokens_addr_map` WRITE;
/*!40000 ALTER TABLE `tokens_addr_map` DISABLE KEYS */;
INSERT INTO `tokens_addr_map` VALUES (1,'10.255.0.2'),(4,'10.5.0.3'),(6,'2.2.2.2'),(7,'10.255.0.5'),(10,'10.5.0.7');
/*!40000 ALTER TABLE `tokens_addr_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `tun_tokens`
--

LOCK TABLES `tun_tokens` WRITE;
/*!40000 ALTER TABLE `tun_tokens` DISABLE KEYS */;
/*!40000 ALTER TABLE `tun_tokens` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `usr_tokens`
--

LOCK TABLES `usr_tokens` WRITE;
/*!40000 ALTER TABLE `usr_tokens` DISABLE KEYS */;
INSERT INTO `usr_tokens` VALUES (6,'nib','a62aacd4d027b2805fa08d0f03ebe250','Nicolas','Bouliane','nib@mind4networks.com',NULL,'2009-02-16 04:57:10'),(3,'saj','bde686993c00a396e1385b1978ad2691','Samuel','Jean','saj@mind4networks.com','a:2:{i:5;i:5;i:6;i:6;}','2008-12-16 00:46:00'),(5,'hplaplante','d4495edd9bccaacc17153814074772b6','Hugues','Laplante','hlaplante@groupelaplante.com','a:1:{i:6;i:6;}','2009-01-30 19:10:43'),(3,'saj-admin','0d29a3ddeb534cd4db8b045aaa073ec5','Samuel','Jean','saj@mind4networks.com','a:2:{i:3;i:3;i:6;i:6;}','2008-12-16 01:04:03'),(5,'sylvain','f4777ec43ff4d700574f5ed01956afc0','Sylvain','','sylvain@groupelaplante.com',NULL,'2009-01-30 19:12:30'),(11,'elis','25813c38cf0ef1596d6e1e1a319c75ba','Elis','Parent','eparent@smakdesign.ca',NULL,'2009-03-09 21:55:42');
/*!40000 ALTER TABLE `usr_tokens` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2009-08-11  0:15:11
