# MiniOS操作系统小型实例

## 0. 项目文件结构

## 1. 操作系统复现

### 1. mbr.S主引导记录
![](https://raw.githubusercontent.com/yetao0806/CloudImage/main/MNIOS20220119202518.png)
### 2. loader.S加载器
![](https://raw.githubusercontent.com/yetao0806/CloudImage/main/MNIOS20220119202633.png)
### 3. 内核文件
![](https://raw.githubusercontent.com/yetao0806/CloudImage/main/MNIOS20220119202706.png)
#### 1. 内存管理
初始化

## 2. 遇到问题
### 线程调度
通过`ret`得到栈中的返回地址执行, 导致第一次执行的`unused_addr`占位符如果是线程只执行一条语句返回,
则会出现`valid opcode exception`
