****************************************************************************
* 文件说明
* 沈汉坤/00130424/2011-09-06
****************************************************************************

****************************************************************************
* 自定义升级文件的配置文件
****************************************************************************
1. 需要打包的升级文件放到该目录下。
2. upgpk.cfg为自定义升级文件的后缀配置, 默认为.upgpk 和.bin
3.自定义升级文件命名格式如下
   前缀_xxx.yy.upgpk 或是 前缀_xxx.yy.bin xxx表示产品形态，比如41为抄控器 1为集中器; 
   yy表示文件版本号，取值为1~255.
   例如 hi3911_ndm_bqfw_49.1.upgpk 前缀为hi3911_ndm_bqfw 49表示BQ升级文件的产品形态自定义,而1表示文件版本号，通过HI_MDM接口(.ucProductN0)可以获取.
4.在 image cfg文件中如 firmware_diag.hbin.hupg.himdl.cfg 加入 hi3911_ndm_bqfw_49.1.upgpk.zpk 
   $(PRJ_CHIP_NAME)_acore.bin.zpk  hi3911_ndm_bqfw_49.1.upgpk.zpk  $(OB_ENV_BUILD_CFG_HNV_PREFIX_NAME)_nv_041_041.hnv.zpk
5.自定义版本设置，例如
  文件 hi3911_na_upg_fake_500.1.upgpk 对应版本号和依赖平台号定在  hi3911_na_upg_fake_501.1.upgpk.cfg文件中
