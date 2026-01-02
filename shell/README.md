# **目標 : 了解pipe的概念和操作方式**

### 建立簡單的shell功能，允許使用單一指令或是多個指令
### 在多個指令下，建立pipe提供指令的輸入/輸出

##
# **功能**

* 支援單一的指令
* 支援多個指令 (在此情況下建立pipe來操作指令的輸入/輸出)

##
# **程式執行**

### **編譯**
1. cd shell
2. make
3. ./main

### **清除**
make clean

### **執行**
```bash
myshell> pwd
myshell> ls
myshell> help
myshell> cat YOUR_FILE_NAME | grep KEY_WORD | wc -l