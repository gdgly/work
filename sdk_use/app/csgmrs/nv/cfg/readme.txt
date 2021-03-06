########################################
# NV文件 升级配置说明 
# 功能：构建升级文件过程中（非SDK）修改NV项参数
# 2012-06-25 s00130424
#
# 格式为 nv_yyy_*.cfg, yyy为数字表示产品形态
######################################

nv_500_xxx.cfg  : 通用的NV升级文件，所有设备都升级, 如用于工厂生产使用的默认值
nv_001_xxx.cfg  : 仅升级 集中器载波模块；版本1；单相耦合
nv_011_xxx.cfg  : 仅升级 集中器载波模块；版本1；三相耦合
nv_101_xxx.cfg  : 仅升级 电表/I型采集器 载波模块；版本1

以上xxx表示HUPG文件适配模块版本允许升级的产品形态

各文件含义说明：
nv_001_001.1.cfg  :   II采工装下CCO的NV配置文件
nv_001_001.41.cfg :   抄控器检验工装对应的cco程序
nv_001_001.cfg    :   CCO的NV配置文件
nv_801_801.cfg    :   STA产线工装下CCO的NV配置
nv_802_802.cfg    :   STA出厂检验工装模式下CCO的NV配置

nv_031_500.cfg    ：  为三相STA的NV配置文件
nv_101_500.cfg	  ：  STA的NV配置文件
nv_101_500.2.cfg  ：  CCO工装陪测STA产品形态
nv_105_500.cfg	  :   I采的NV配置文件
nv_105_500.1.cfg  :   I采在产线工装模式下NV配置
nv_111_500.cfg	  ：  II型采集器的NV配置文件 
nv_500_111.cfg    :   II采模块在产线模式下NV配置
nv_500_500.cfg    :   STA在产线工装模式下NV配置

nv_061_061.cfg	  ：  三代抄控器的NV配置文件


注意：cfg文件中各ID表示请尽量以固定格式显示以便后续维护，此处规定以有效16进制数据开始表示，示例如下：
正确表示方式：ID = 0xA
错误表示方式：ID = 10 或 ID = 0x0A



在cfg文件中增加注释的注意事项：
1、仅支持英文注释；
2、注释不要写在ID及PARAM_VALUE行；
3、不要在ID及PARAM_VALUE两行中间插入注释；
4、注释行中间不能有“ = ”号；
5、建议注释放在ID行的上一行，用“#”号开头。