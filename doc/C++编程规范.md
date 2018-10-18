# C++编程规范

> tinoryj

## 命名规则    
### 类名
大驼峰法：

```c++
class MyPoint;
```   
### 变量名、方法名
小驼峰法：

```c++
int arrayCount;  
MyPoint centerPoint;   
string getError();
```   

### enum枚举类中的成员
大驼峰法:
  
```c++
enum MyColor {Red, Yellow, Blue, Black, LightBlue};
``` 
  
### const常量和宏
每个字母大写,独立单词间用`_`连接:

```c++
const double MAX_VALUE; 
#define MY_SIZE 10
```   
### 类方法
##### public function
标准小驼峰法：

```c++
public:
    void initDB();
```

##### private function
小驼峰法加前缀`_`:

```c++
private:    
    void _checkHelper();
```
   
##### protected function
小驼峰法加前缀`__`(连续两个`_`):

```c++ 
protected:  
    string __getErroeHelper();
```

## 程序规范
### 代码段
1.相对独立的代码块之间加空行。花括号左半不单独分行，与前一个符号用一个空格隔开；右半单独占有一行。

```c++
if (condition) {
    //code
}
//空行
int numCount = 0;
```
2.长度过长的语句（大于80字符），应在优先级较低的操作符处分行。
3.`if,for,do,while,case,switch,default`等语句自占一行,且执行语句部分无论多少都要加括号{}。

### 空格和Tab
1.逗号、分号处在其后空一个空格。

```c++
int a, b, c;
```
2.双目操作符前后均添加一个空格。

```c++
uint64_t ans = (a * x + b) / m;
if (flag0 == true && num0 == 10) {
    //code
}
```

3.单目操作符前后不添加空格。

```c++
bool flag = !isEmpty;
i++;
```
4.`if,for,while,switch`等与后面的括号间加一个空格
5.多层嵌套时的Tab（同时不同层之间空行）：

```c++
for (int i = 0; i < MAX_NUM; i++) {
    
    for (int j = 0; j <= i; j++) {
        
        if (num[i][j] == 0) {
            
            break;
        }
    }
}
```

### 注释
1.尽量使用英文注释（中文非等线字体加在代码里会比较难看）。
2.对每一个重要（可能会复用到的）模块通过多行注释解释清楚函数名、依赖的函数、输入、输出等信息。
3.对于函数内部部分代码块的含义，在其上方单独行添加注释，若是单句，则在该句后添加注释，一份代码完成后将单行注释开始的位置对齐。

```c++
/*
convert hash string to an long int number (base 16);
input : hash(string);
output : number(long int);
*/
long int chunkHashCal(string hash){
    /* erase ':' form hash string */
    hash.erase(std::remove(hash.begin(), hash.end(), ':'), hash.end());
    char * pEnd;    //strtol function's end flag
    long int num;   //the number will return
    num = strtol(hash.c_str(), &pEnd, 16);
    return num;
}
```

## 项目管理

1.参与项目的成员，通过git创建属于自己的分支。
2.当当前功能、模块出现bug时建立debug分支，修复bug后将该debug分支merge到自己的分支中，并删除该debug分支。
3.当成员自己的模块功能测试稳定后，发起pr，由管理员（项目负责人）检查，符合条件后merge到master分支中。
4.项目成员（非管理员）不得直接更改master分支中的内容，以避免冲突。

>当两条分支对同一个文件的同一个文本块进行了不同的修改，并试图合并时，Git不能自动合并的，称之为冲突(conflict)。解决冲突需要人工处理。 


