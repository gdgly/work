//////////////////////////////////////////////////////////////////////////////
// 文件说明
// 沈汉坤/00130424/2011-09-06
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// APP: 应用层代码
// 包括应用协议(376/645), 和 HSO对接的应用层代码, 升级, 以及其他的产品应用代码
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
目录说明
//////////////////////////////////////////////////////////////////////////////
目录名称为子模块的简称, 如下说明:
mrs         : 抄表协议代码
dfx         : 可维可测代码,包括和HSO对接的代码
equ         : 产线相关功能代码
init        : APP初始化处理代码 
misc        : 其它功能代码
upg         : 升级代码
inc         : APP目录下各个子模块的公用接口定义的头文件, 仅供APP目录内部的子模块使用
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// 
// 目录和文件使用说明
//
//////////////////////////////////////////////////////////////////////////////
1. 新加文件,统一以 <子模块简称>_<功能简述>.c/.h
2. 新加文件, 统一包含头文件 app_config.h 即加入代码行 #include "app_config.h"
//////////////////////////////////////////////////////////////////////////////


