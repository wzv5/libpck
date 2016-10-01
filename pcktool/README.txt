解压所有文件：
pcktool -x input.pck

解压指定文件：
pcktool -x input.pck 排除列表.txt 保留列表.txt
如果以“\”结尾，则为目录，否则为文件。
如果行首为“#”，则该行为注释，不作为列表内容。

压缩：
pcktool -c output.pck inputdir

列出所有文件：
pcktool -l input.pck

感谢 stsm / liqf / 李秋枫 开源的WinPCK！
