# tiny_daemon

轻量级的进程管理服务，底层依靠`start-stop-daemon`实现，支持服务的启动、停止、重启、状态查询等功能，自动重启功能

tinysystemd: 进程管理服务器
tinysystemd_ctl: 进程管理工具

## tinysystemd

tinysystemd 运行之前必须有 service_dir文件夹

启动参数

```
tinysystemd [options]
--service_dir=<services_dir>  # 服务目录 默认 /etc/tiny_daemon/services
--pid_dir=<pid_dir>          # pid目录 默认 /var/run/tiny_daemon
```

## tinysystemd_ctl

tinysystemd_ctl 必须在tinysystemd运行之后才能使用

```
tinysystemd_ctl list  # 列出所有服务和状态
tinysystemd_ctl reload  # 重新加载配置
tinysystemd_ctl start <service_name>  # 启动服务
tinysystemd_ctl stop <service_name>  # 停止服务
tinysystemd_ctl restart <service_name>  # 重启服务
```