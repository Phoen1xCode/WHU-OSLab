#!/usr/bin/perl -w

#
# usys.pl - 生成用户态系统调用存根 (usys.S)
#
# 用法: perl usys.pl > usys.S
#
# 生成的汇编代码为每个系统调用创建一个函数：
# 1. 将系统调用号加载到 a7 寄存器
# 2. 执行 ecall 指令陷入内核
# 3. 返回（返回值在 a0 寄存器中）
#

print "# usys.S - 用户态系统调用存根\n";
print "# 由 usys.pl 自动生成，请勿手动编辑\n";
print "#\n";
print "# 每个系统调用函数：\n";
print "#   1. 将系统调用号放入 a7 寄存器\n";
print "#   2. 执行 ecall 触发陷入内核\n";
print "#   3. 返回，返回值在 a0 中\n";
print "#\n\n";

print "#include \"kernel/syscall.h\"\n\n";

# 生成系统调用存根
sub entry {
    my $name = shift;
    print ".global $name\n";
    print "$name:\n";
    print "    li a7, SYS_${name}\n";
    print "    ecall\n";
    print "    ret\n\n";
}

# 进程相关系统调用
entry("fork");
entry("exit");
entry("wait");
entry("getpid");
entry("kill");
entry("sleep");

# 内存相关系统调用
entry("sbrk");

# 系统信息
entry("uptime");

# I/O 相关
entry("write");
entry("read");

# 文件系统相关
entry("open");
entry("close");
entry("unlink");
entry("link");
entry("mkdir");
entry("chdir");
entry("dup");
entry("pipe");
entry("fstat");
entry("mknod");

entry("hello");  # 自定义系统调用