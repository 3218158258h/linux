### mount 命令
用于挂载文件系统（如硬盘、U盘、网络存储等）到Linux目录。

##### 1. 基础用法
- 挂载设备到目录：`sudo mount <设备路径> <挂载点>`  
  示例：`sudo mount /dev/sdb1 /mnt/usb`（挂载U盘到/mnt/usb）

- 查看已挂载的文件系统：`mount`（不加参数）

#### 2. 常用选项
- 挂载ISO镜像：`sudo mount -o loop <镜像文件> <挂载点>`  
- 卸载（对应操作）：`sudo umount <挂载点>`（注意是umount，无n）

#### 注意
- 挂载点目录需先存在（如`mkdir /mnt/usb`）
- 通常需要sudo权限

### man 命令
#### 1. 基础查询（最常用）
- 查命令手册：`man <命令名>`  
  示例：`man ls`（查 ls 用法）、`man apt`（查 apt 用法）
#### 2. 手册内关键操作
- 搜索关键词：输入 `/关键词` 回车（`n` 下一个，`N` 上一个）  
- 退出手册：按 `q` 键

### apt 核心常用命令

#### 1. 软件源与更新
- 更新软件包列表：`sudo apt update`  
- 升级所有软件：`sudo apt upgrade`  


#### 2. 软件包管理
- 安装软件：`sudo apt install <包名>`  
- 卸载软件（保留配置）：`sudo apt remove <包名>`  
- 彻底卸载（含配置）：`sudo apt purge <包名>`  
- 搜索软件：`apt search <关键词>`  

#### 3. 系统清理
- 清理缓存安装包：`sudo apt clean`  
- 自动移除无用依赖：`sudo apt autoremove`

##别名
- 临时生效：`alias 别名='原命令'`（如 `alias ll='ls -l'`）
- 查看别名：直接输入 `alias`
- 删除别名：`unalias 别名`
- 永久生效：将别名写入 `~/.bashrc`（bash）或 `~/.zshrc`（zsh），再执行 `source 文件名` 生效
## 文件/目录操作

### ls - 列出目录内容
```bash
ls                # 列出当前目录可见文件
ls -a             # 显示所有文件（包括隐藏文件）
ls -l             # 长格式显示详细信息（权限、大小、时间等）
ls -lh            # 人性化显示文件大小（KB、MB等）
ls -lt            # 按修改时间排序（最新在前）
ls /path/dir      # 列出指定目录内容
ls -r             # 递归地列出
```

### rm - 删除文件/目录
```bash
rm file.txt       # 删除文件
rm -r dir/        # 递归删除目录及内容
rm -f file.txt    # 强制删除，不提示
rm -rf dir/       # 强制递归删除目录（慎用）
rm -i *.txt       # 删除前逐一确认
```

### cp - 复制文件/目录
```bash
cp src.txt dest.txt   # 复制文件
cp src.txt dir/       # 复制文件到目录
cp -r dir1/ dir2/     # 递归复制目录
cp -i src.txt dest/   # 覆盖前提示
cp -v file.txt backup/ # 显示复制过程
```

### mv - 移动/重命名文件/目录
```bash
mv old.txt new.txt    # 重命名文件
mv file.txt dir/      # 移动文件到目录
mv dir1/ dir2/        # 移动或重命名目录
mv -f file.txt dest/  # 强制覆盖
mv -v *.txt docs/     # 显示移动过程
```

## 文本处理

### cat - 查看/创建文本文件
```bash
cat file.txt          # 查看文件内容
cat file1.txt file2.txt  # 合并显示多个文件
cat > new.txt         # 创建新文件（Ctrl+D结束输入）
cat >> file.txt       # 追加内容到文件
cat -n file.txt       # 显示内容并带行号
```

### grep - 文本搜索
```bash
一般grep "abc" -nwr即可满足日常使用 
grep "pattern" file.txt  # 在文件中搜索模式
grep -i "pattern" file.txt  # 忽略大小写
grep -n "pattern" file.txt  # 显示匹配行号
grep -r "pattern" dir/    # 递归搜索目录
grep -v "pattern" file.txt  # 显示不匹配的行
grep -w "word" file.txt   # 精确匹配单词
```
## 权限管理
### chmod - 修改权限
```bash
chmod u+x file.sh     # 给所有者添加执行权限
chmod g-w file.txt    # 移除组的写权限
chmod o=r file.txt    # 给其他人只读权限
chmod 755 file.sh     # 所有者: rwx，组和其他人: r-x
chmod 644 file.txt    # 所有者: rw-，组和其他人: r--
chmod -R 755 dir/     # 递归修改目录权限
```
### chown - 修改所有者/所属组
```bash
chown user file.txt   # 修改文件所有者
chown user:group file.txt  # 修改所有者和所属组
chown :group file.txt  # 仅修改所属组
chown -R user:group dir/  # 递归修改目录
```
## 搜索与查找
### find - 查找文件/目录
```bash
find /path -name "file.txt"  # 按名称查找
find ./ -name "*.txt"        # 查找所有txt文件
find /path -type d -name "dir"  # 查找目录
find ./ -type f -size +100M  # 查找大于100MB的文件
find ./ -mtime -7            # 查找7天内修改的文件
find ./ -name "*.tmp" -delete  # 删除找到的文件
```
## 压缩与归档
### tar - 打包/解包（常与压缩结合）
```bash
-c代表归档，加上z、j代表压缩（不同方式）
-x代表解包，即解压
-f代表file，要放在最后
-v可以省略，区别在于是否输出详细信息，不影响功能
tar -cvf archive.tar dir/  # 打包目录（不压缩）
tar -zcvf archive.tar.gz dir/  # 打包并gzip压缩
tar -jcvf archive.tar.bz2 dir/ # 打包并bzip2压缩（压缩了更高）
tar -xvf archive.tar       # 解包
tar -zxvf archive.tar.gz   # 解压tar.gz文件
tar -xvf archive.tar -C /path  # 解压到指定目录
tar -tvf archive.tar       # 查看归档内容（不解压）
```
