#include "analyseXml.h"


//全局变量的声明

extern stringstream framess;
extern ofstream logFile;
extern stringstream frameobj;
extern stringstream totalFrame;

// chen
vector<string> xmlContent;	// 存放xml中应显示的内容
vector<string> addInXml;	// 存放每个obj中的transfrom属性
vector<string> abcProp;		// 存放每个abc的属性（文件名除外）
size_t tabNum;	// 单行增加制表符，用于控制格式
bool hasOutputCamera;



//walk,visit函数是参考abcImPort的方法

void walk(Alembic::Abc::IArchive & iRoot)
{
	IObject mParent = iRoot.getTop();


	if (!iRoot.valid())
	{
		cout << "文档无效，没有根节点！" << '\n';
		exit(0);
	}
	//预加载缓存层次结构
	AlembicObjectPtr topObject =
		previsit(AlembicObjectPtr(new AlembicObject(iRoot.getTop())));

	if (!topObject)
		cout << "topObject为空" << '\n';

	size_t numChildren = topObject->getNumChildren();
	if (numChildren == 0)
	{
		//cout << "numChildren为空" << endl;
	}

	for (size_t i = 0; i < numChildren; i++)
	{	
		visit(topObject->getChild(i));
	}
}

//手动编写参数
void subdivide(const string &filePath) // chen
{
	try
	{
		Alembic::AbcCoreFactory::IFactory factory;
		IArchive archive = factory.getArchive(filePath);
		walk(archive);
	}//try结束的地方
	catch (const exception & e)
	{
		cerr << e.what() << '\n';
		cerr << "AlembicRiProcedural: cannot read arguments file: ";
		cerr << filePath << '\n';
	}
}


void xmlAnalyze(const string &xmlFile)
{
	int abcNum=0;
	ifstream fin(xmlFile.c_str());
	if(!fin)
	{
		cout<<"没有找到文件！\n";
		exit(0);
	}
	string line; 

	//下面的代码是循环遍历找abc标签
	while (getline(fin, line)) {
		////解析abc标签
		//如果没有找到abc标签
		if(line.find("<shape type=\"abc\">") == string::npos)
		{
			xmlContent.push_back(line);
		}else {
			string abcFile;
			abcProp.clear();
			abcNum++;
			tabNum = line.find("<shape type=\"abc\">");
			while (getline(fin, line) && line.find("</shape>") == string::npos) {
				if (line.find("name=\"filename\"") != string::npos) {
					size_t pos = line.find("value");
					abcFile = line.substr(pos+7, line.size()-pos-10);
					cout<<"abcfile:"<<abcFile<<'\n';
				}	
				else
					abcProp.push_back(line);//将abc标签中的名字加入属性中	
			}
	
			subdivide(objPath+abcFile);//相对路径,自己加上objPath（解析找到的每一个abc文件）
			//subdivide(abcFile);
		}


		////解析相机的标签
		if(line.find("<sensorabc type=\"abc\">") != string::npos)
		{
			xmlContent.pop_back();
			string abcFile;
			abcProp.clear();
			abcNum++;
			tabNum = line.find("<sensorabc type=\"abc\">");
			hasOutputCamera=true;
			while (getline(fin, line) && line.find("</sensorabc>") == string::npos) {
				if (line.find("name=\"filename\"") != string::npos) {
					size_t pos = line.find("value");
					abcFile = line.substr(pos+7, line.size()-pos-10); 
					cout<<"abcfile:"<<abcFile<<'\n';
				}	
				else
					abcProp.push_back(line);//将abc标签中的名字加入属性中	
			}
	
			subdivide(objPath+abcFile);//相对路径,自己加上objPath（解析找到的每一个abc文件）
			//subdivide(abcFile);

			//写入相机信息(这个要放在里面，放在外面有错误)
	        writeSensor();

			//写入max的相机信息
//			writeSensorMax();
		}



	}//while循环结束

	
	//如果xml文件中没有abc标签，程序就退出
	if(abcNum==0)
	{
		cout<<"没有abc文件，程序退出"<<'\n';
		exit(0);
	}
}
