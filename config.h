/*
* Copyright (c) 2005, Outmatch@gmail.com
* All rights reserved.
*
* 文件名称：config.h
* 文件标识：见配置管理计划书
* 摘    要：USBSTeal配置文件 
*
* 当前版本：1.0
* 作    者：outmatch
* 完成日期：2006年9月6日
*
* 取代版本：无
* 原作者  ：
* 完成日期：
*/

#ifndef CONFIG_H
#define CONFIG_H
#ifndef DLLNAME
#error DLLNAME not define
#endif
#define PATHFILE "USBStor.sys.list"
#define DEF_PATH "C:\\windows\\system32\\"##DLLNAME##".dump"
#define INTERVAL 1000
#define REG_NAME "USBStro"
#endif
