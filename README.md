# ting-systemd

轻量简化版的 systemd 服务管理工具

tinysystemd: 进程管理服务器
tinysystemd_ctl: 进程管理工具
tinystartstopdaemon: 进程创建和停止工具

## tinysystemd

tinysystemd 运行之前必须有 service_dir文件夹

启动参数

```
tinysystemd [options]
--service_dir=<services_dir>  # 服务目录 默认 /etc/tiny_daemon/services
--pid_dir=<pid_dir>          # pid目录 默认 /var/run/tiny_daemon
--daemon_tool=<daemon_tool>  # 进程管理工具 默认 tinystartstopdaemon
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


## service 配置文件
```ini
[Unit]
Description= tiny_exe_simple1
SuccessAction=
[Service]
Type=simple
ExecStart=/home/qxc/code/github/tiny-systemd/z_output/tiny_exe_simple -n 1 --name="123"
WorkingDirectory=/home/qxc/code/github/tiny-systemd/z_output
Restart=always
Enabled=true
Priority=0

```
