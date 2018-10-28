#include "analyseXml.h"
#include "procMain.h"


extern stringstream framess;
extern ofstream logFile;
extern string logname;
extern stringstream frameobj;
extern stringstream totalFrame;

extern vector<string> xmlContent;	// 存放xml中应显示的内容
extern vector<string> addInXml;	// 存放每个obj中的transfrom属性
extern vector<string> abcProp;		// 存放每个abc的属性（文件名除外）
extern size_t tabNum;	// 单行增加制表符，用于控制格式

//用来写帧文件的信息（一共有多少帧，每一帧xml文件对应的路径）
ofstream infofile;
string infoname;




//生成每一帧的xml以及obj文件
void generatexml(string xmlPath,double frame1)
{
	objCount=0;
	objPath=xmlPath.substr(0,xmlPath.find_last_of("\\"))+"\\";

	char tmp[40];
    time_t t=time(0);
    strftime( tmp, sizeof(tmp), "%Y-%m-%d %X",localtime(&t) );

	frameobj.clear();
	frameobj.str("");
	frameobj<<frame1;

	//解析程序开始，打印log
	logname=objPath+frameobj.str()+".log";
	

	logFile.open((logname).c_str());
	logFile<<tmp<<"\tstart"<< '\n';

	string xmlFile;
	xmlFile=xmlPath;
	cout<<xmlFile<<'\n';

	frame=frame1;
	cout<<frame<<'\n';//用来控制Obj文件的帧数

	xmlAnalyze(xmlFile);


	framess.clear();
	framess.str("");
	framess<<frame;
	string framename=objPath+"frame"+framess.str()+".xml";
	//cout<<"framename:"<<framename<<endl;
	ofstream fout(framename.c_str());
	for (size_t i = 0; i != xmlContent.size(); ++i) 
		fout << xmlContent[i] << '\n';


	infofile.open(infoname.c_str(),ios::app);
	infofile<<framename<<'\n';
	infofile.close();

	

	char tmpfinal[40];
    time_t tfinal=time(0);
    strftime( tmpfinal, sizeof(tmpfinal), "%Y-%m-%d %X",localtime(&tfinal) );
    logFile<<tmpfinal<<"\tcompleted"<<'\n';
    logFile.close();

}

////使用命令行
int main(int argc,char** argv)
{
	generatexml(argv[1],atoi(argv[2]));
}

