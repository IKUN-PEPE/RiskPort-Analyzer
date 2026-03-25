PortScanner/
├── main.cpp          # 程序入口、消息循环
├── resource.h        # 所有控件ID、颜色、尺寸常量
├── data.h / data.cpp # 高危端口数据库 + ActivePort结构体
├── scanner.h/.cpp    # 端口扫描逻辑（工作线程）
├── actions.h/.cpp    # 业务操作：终止进程、防火墙规则
├── ui_theme.h/.cpp   # 自绘UI：背景、按钮、Banner、配色
└── ui_window.h/.cpp  # 窗口过程、控件创建、ListView刷新
