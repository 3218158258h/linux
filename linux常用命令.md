# Linux 常用命令速查表

## 文件/目录操作

### ls - 列出目录内容
```bash
ls                # 列出当前目录可见文件
ls -a             # 显示所有文件（包括隐藏文件）
ls -l             # 长格式显示详细信息（权限、大小、时间等）
ls -lh            # 人性化显示文件大小（KB、MB等）
ls -lt            # 按修改时间排序（最新在前）
ls /path/dir      # 列出指定目录内容
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

### gzip - 压缩/解压文件
```bash
gzip file.txt         # 压缩文件（生成file.txt.gz）
gzip -d file.txt.gz   # 解压文件
gunzip file.txt.gz    # 解压（gzip -d的别名）
gzip -c file.txt > file.txt.gz  # 压缩并保留原文件
```

### tar - 打包/解包（常与压缩结合）
```bash
tar -cvf archive.tar dir/  # 打包目录（不压缩）
tar -zcvf archive.tar.gz dir/  # 打包并gzip压缩
tar -jcvf archive.tar.bz2 dir/ # 打包并bzip2压缩
tar -xvf archive.tar       # 解包
tar -zxvf archive.tar.gz   # 解压tar.gz文件
tar -xvf archive.tar -C /path  # 解压到指定目录
tar -tvf archive.tar       # 查看归档内容（不解压）
```
