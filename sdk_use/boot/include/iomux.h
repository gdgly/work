#ifndef __IOMUX_H__
#define __IOMUX_H__

#include<types.h>

typedef enum
{
	HI_HW_IO0,
	HI_HW_IO1,
	HI_HW_IO2,
	HI_HW_IO3,	
	HI_HW_IO4,
	HI_HW_IO5,
	HI_HW_IO6,
	HI_HW_IO7,
	HI_HW_IO8,
	HI_HW_IO9,
	HI_HW_IO10,
	HI_HW_IO11,
	HI_HW_IO12,
	HI_HW_IO13,
	HI_HW_IO14,
	HI_HW_IO15,
	HI_HW_IO16,
	HI_HW_IO17,
	HI_HW_IO18, 
	HI_HW_IO19,
	HI_HW_IO20,
	HI_HW_IO21, 
	HI_HW_IO22,
	HI_HW_IO23,
	HI_HW_IO24,
	HI_HW_IO25,
	HI_HW_IO26,// ��1����ַ
	HI_HW_IO27,
	HI_HW_IO28,
	HI_HW_IO29,
	HI_HW_IO30,
	HI_HW_IO31,
    HI_HW_IO9_BACKUP,
    HI_HW_IO10_BACKUP,
    HI_HW_IO11_BACKUP,
    HI_HW_IO12_BACKUP,
    HI_HW_IO13_BACKUP,
    HI_HW_IO14_BACKUP,    
    HI_HW_IO15_BACKUP,
    HI_HW_IO16_BACKUP,// ��25����ַ
    HI_HW_IO17_BACKUP,
    HI_HW_IO18_BACKUP,
    HI_HW_IO19_BACKUP,
    HI_HW_IO20_BACKUP,    
    HI_HW_IO26_BACKUP,  
    HI_HW_IO27_BACKUP, 
    HI_HW_IO28_BACKUP,
    HI_HW_IO29_BACKUP,  
	HI_HW_IO_MAX,
}HI_HW_IO_E;

u32 iomux_config(HI_HW_IO_E enIoIdx, u8 ulIoVal);

#endif//__IOMUX_H__