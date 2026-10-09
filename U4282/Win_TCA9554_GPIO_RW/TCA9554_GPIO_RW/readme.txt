工具名称：TCA9554_GPIO_RW.exe

工具类型：控制TCA9554芯片出的GPIO工具，Windows命令行程序，需要管理员身份运行CMD

使用说明：<TCA_GPIO_RW.exe> <-w/-r> <address> <GPI/GPO> <IONUM> <0/1>

命令举例：
          TCA_GPIO_RW.exe -w GPO 0x72 4 1   //向地址/位数 0x72/4 的GPO写入高电平
          TCA_GPIO_RW.exe -r GPO 0x72 5     //读取地址/位数 0x72/5 的GPO的电平值
	  TCA_GPIO_RW.exe -r GPI 0x72 0     //读取地址/位数 0x72/0 的GPI的电平值
