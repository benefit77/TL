工具名称：TCA_GPIO_RW

工具类型：控制TCA9539、TCA9554、TCA9555芯片出的GPIO工具，linux命令行程序，需要管理员身份运行

使用说明：<./TCA_GPIO_RW> <-w/-r> <9539/9554/9555> <address> <GPI/GPO> <IONUM> <0/1>

命令举例：
          ./TCA_GPIO_RW -w 9554 0x42 GPO 7 1  //对tca9554芯片，SMBUS地址为0x42的GPO7写入1，即高电平
	  ./TCA_GPIO_RW -r 9554 0x42 GPI 2    //读取tca9554芯片，SMBUS地址为0x42的GPI2的状态
	  ./TCA_GPIO_RW -h       //打印帮助信息                         

