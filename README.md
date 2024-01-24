## 说明
- 本程序参考TinyHttpd项目实现了一个简易版http服务，功能较简单，主要帮助理解http协议
- [TinyHttpd](https://github.com/EZLippi/Tinyhttpd)
- 并且包含一个轻量级json处理库
- [json11](https://github.com/dropbox/json11)

## 编译
- 创建build目录，在build目录下执行以下命令
- **Windows平台**
	>- cmake -G "Visual Studio 14 2015" ..
	>- cmake --build ./ --config Release
- **Linux平台**
	>- sudo cmake ..
	>- sudo make