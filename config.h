/*
* Copyright (c) 2005, Outmatch@gmail.com
* All rights reserved.
*
* �ļ����ƣ�config.h
* �ļ���ʶ�������ù���ƻ���
* ժ    Ҫ��USBSTeal�����ļ� 
*
* ��ǰ�汾��1.0
* ��    �ߣ�outmatch
* ������ڣ�2006��9��6��
*
* ȡ���汾����
* ԭ����  ��
* ������ڣ�
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
