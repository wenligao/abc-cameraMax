#include <string>
#include <iostream>
#include <vector>
#include <sstream>
using namespace std;
//用来定义全局变量

string objPath;//用来存放生成的Obj文件
string xformpath;
//string xmlFile;
stringstream ss;//用来控制obj文件的数目
stringstream framess;//用来控制xml文件所对应的帧数（整数到字符串的转换）
stringstream  frameobj;//每一个obj所对应的帧数
stringstream totalFrame;//用来控制输出日志中的总帧数
string logname ;


double frame;//指定起始帧
int objCount;//用来统计场景中的obj的数目
