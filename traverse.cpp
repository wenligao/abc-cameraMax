#include <fstream>
#include <iostream>
#include <time.h>
#include "global.h"
#include "animatePorcess.h"
#include "SampleUtil.h"
#include "analyseXml.h"
using namespace std;

ofstream logFile;
extern string logname;

// chen
extern vector<string> addInXml; 
extern vector<string> xmlContent;
extern vector<string> abcProp;
extern size_t tabNum;
extern stringstream ss;//用来控制obj文件名的数目
extern stringstream framess;//用来控制帧的数目
extern stringstream frameobj;//用来控制obj文件的帧的数目
vector<string> cameraProp;	//相机参数
extern bool hasOutputCamera;

// 从xmlContent中找到相机的信息，返回它在xmlContent中的位置
int findDefaultCamera(vector<string> &xmlContent)
{
	for (int i = 0; i != xmlContent.size(); ++i) {
		if (xmlContent[i].find("sensor") != string::npos) {
			while (i != xmlContent.size() && xmlContent[i].find("</sensor>") == string::npos) {
				if (xmlContent[i].find("lookat") != string::npos) 
					return i;
				++i;
			}

			cout << "未找到</sensor>，或未找到lookat" << '\n';
			return -1;
		}
	}

	cout << "未找到sensor标签" << '\n';
	return -1;
}


//尝试把相机的信息写入
void writeSensor()
{
		// cout << "IPolyMesh" << endl;
		// 匹配IPolyMesh时，说明相机参数已收集结束，需要把它加入xmlContent
		// 但要注意一点格式上的改变
		// 此外还需要注意最后一次加入cameraProp的必定是obj信息，要将其删去
		//在原来的基础上修改，针对特定的max相机，max中有两个参数origin以及target
		vector<string > lookat;
		if (hasOutputCamera && !cameraProp.empty()) {
			//cameraProp.pop_back();
			int index = findDefaultCamera(xmlContent);
			// 相机信息不能为空，且xml中必须有lookat信息
			if (!cameraProp.empty() && index != -1) {
				string cameraTransformType[] = { " origin=" , " target=" , " up=" };
				string toDelete[] = { "<translate x=" , "\" y=\"" , "\" z=\"" , "/>" };
				string camP("<lookat"), &defut(xmlContent[index]);
				for (int i = 0; i != cameraProp.size(); ++i) {
					string &t = cameraProp[i];
					t.erase(t.find(toDelete[0]), toDelete[0].size());	// 删除<translate x=
					t.replace(t.find(toDelete[1]), toDelete[1].size(), " ");	// 把" y="替换成空格
					t.replace(t.find(toDelete[2]), toDelete[2].size(), " ");	// 与上面同理
					t.erase(t.find(toDelete[3]), toDelete[3].size());	// 与第一个erase同理
					lookat.push_back(t);
				}
				for(int i=0;i != cameraProp.size(); i++)
				{
					if(i==0)
					{
						camP += (cameraTransformType[i] + lookat[1]);
					}else
						camP += (cameraTransformType[i] + lookat[0]);
				}
				

				//如果参数数量不够就要把默认值填补到后面
				if (cameraProp.size() == 1)
					camP += defut.substr(defut.find(cameraTransformType[1]));
				else if (cameraProp.size() == 2)
				{
					//camP += defut.substr(defut.find(cameraTransformType[2]));
					//如果只有两个相机参数，默认up参数为0 1.0 0
					camP += " up= \"0 1.0 0\"/>";
				}
				else
					camP += toDelete[3];
				defut = string(tabNum + 2, '\t') + camP;

				cout<<"camtype2==="<<cameraTransformType[2]<<endl;
				cout<<"camp====="<<camP<<endl;
				cout<<"defut===="<<defut<<endl;
			}
		}//相机的if语句结束
}


//预加载缓存文档
AlembicObjectPtr previsit(AlembicObjectPtr iParentObject)
{
	Alembic::Abc::IObject parent = iParentObject->object();
	const string name = parent.getFullName().c_str();
	const size_t numChildren = parent.getNumChildren();
	
	for (size_t i = 0; i < numChildren; i++)
	{
		Alembic::Abc::IObject child = parent.getChild(i);
		AlembicObjectPtr childObject =
			previsit(AlembicObjectPtr(new AlembicObject(child)));

		if (childObject)
		{
			iParentObject->addChild(childObject);
		}
	}
	return iParentObject;
}

void visit(AlembicObjectPtr iObject)
{
	Alembic::Abc::IObject iObj = iObject->object();
	if (Alembic::AbcGeom::IXform::matches(iObj.getHeader()))
	{
		//在每次匹配之前都要清除
		addInXml.clear();
		Alembic::AbcGeom::IXform xform(iObj, Alembic::Abc::kWrapExisting);
	
		//下面的代码是读取文档中IXform中的信息(参考prman中的代码ProcessXform)
		Alembic::AbcGeom::IXformSchema &xformFS = xform.getSchema();//获取该对象对应的模式
		TimeSamplingPtr ts = xformFS.getTimeSampling();
		size_t xformSamps = xformFS.getNumSamples();
		//cout<<"numsamples:"<<xformSamps<<'\n';
	
		SampleTimeSet sampleTimes;
		GetRelevantSampleTimes(ts, xformSamps, sampleTimes);

		//cout<<"sampleTimes:"<<sampleTimes.size()<<'\n';
		bool multiSample = sampleTimes.size()>1;
		vector<XformSample> sampleVectors;
		sampleVectors.resize(sampleTimes.size());

		size_t sampleTimeIndex = 0;
		for (SampleTimeSet::iterator I = sampleTimes.begin();
			I != sampleTimes.end(); ++I, ++sampleTimeIndex)
		{
			ISampleSelector sampleSelector(*I);
			xformFS.get(sampleVectors[sampleTimeIndex], sampleSelector);

		}//for循环结束

		for (size_t i = 0, e = xformFS.getNumOps(); i < e; ++i)
		{
			if (multiSample)
			{
				WriteMotionBegin(sampleTimes);
				cout<<"multiSample为True"<<'\n';
			}
			for (size_t j = 0; j < sampleVectors.size(); ++j)
			{
				XformOp &op = sampleVectors[j][i];
				//cout<<"op.getType()=="<<op.getType()<<endl;
				switch (op.getType())
				{
				case kScaleOperation:
					{
						V3d value = op.getScale();
						// chen
						ostringstream oss;
						oss << "<scale x=\"" << value.x << "\" y=\""<< value.y << "\" z=\"" << value.z <<"\"/>";
						//cout << "<scale x=\"" << value.x << "\" y=\""<< value.y << "\" z=\"" << value.z <<"\"/>"<<'\n';
						addInXml.push_back(oss.str());
					}
					break;

				case kTranslateOperation:
					{
						V3d value = op.getTranslate();
						// chen
						ostringstream oss;
						oss << "<translate x=\"" << value.x << "\" y=\""<< value.y << "\" z=\"" << value.z <<"\"/>";
						//cout << "<translate x=\"" << value.x << "\" y=\""<< value.y << "\" z=\"" << value.z <<"\"/>"<<'\n';
						addInXml.push_back(oss.str());
					}
					break;

				case kRotateOperation:
				case kRotateXOperation:
					{
						V3d axis = op.getAxis();
						float degrees = op.getAngle();

						// chen
						ostringstream oss;
						oss << "<rotate x=\"" << axis.x << "\" angle=\""<< degrees << "\"/>";
						//cout << "<rotate x=\"" << axis.x << "\" angle=\""<< degrees << "\"/>"<<'\n';
						addInXml.push_back(oss.str());
					}
					break;
				
				case kRotateYOperation:
					{
						V3d axis = op.getAxis();
						float degrees = op.getAngle();

						// chen
						ostringstream oss;
						oss << "<rotate y=\"" << axis.y << "\" angle=\""<< degrees << "\"/>";
						//cout << "<rotate y=\"" << axis.y << "\" angle=\""<< degrees << "\"/>"<<'\n';
						addInXml.push_back(oss.str());
					}
					break;
					
				case kRotateZOperation:
					{
						V3d axis = op.getAxis();
						float degrees = op.getAngle();

						// chen
						ostringstream oss;
						oss << "<rotate z=\"" << axis.z << "\" angle=\""<< degrees << "\"/>";
						//cout << "<rotate z=\"" << axis.z << "\" angle=\""<< degrees << "\"/>"<<'\n';
						addInXml.push_back(oss.str());
					}
					break;

				}//switch语句结束
			}
		}

		// chen
		// 在相机未被输出时，我们应默认所有信息都是相机的（相机默认为第一输出）
		// 可是不能像上面obj那样直接写入xml，因为不确定相机的参数会有几项
		// 所以要用一个vector暂存，如果之后匹配到了IPolyMesh，就说明不再有相机参数了
		// 这里相机只收集第一个transform信息
		//如果是相机的信息，就把变换信息加入到相机的容器里面
		//hasOutputCamera是个全局变量，在最前面找到动态相机的标签时，需要将其置为true
		//这个是用来读Maya中相机的结构
		if (hasOutputCamera && !addInXml.empty())
		{
			cameraProp.push_back(addInXml[0]);
		}

		size_t numChildren = iObject->getNumChildren();
		for (size_t i = 0; i<numChildren; i++)
		{
			visit(iObject->getChild(i));
			
		}
	}
	//循环遍历场景中的节点
	else if (Alembic::AbcGeom::ISubD::matches(iObj.getHeader()))
	{
		// chen
		//cout << "IsubD" << endl;
		Alembic::AbcGeom::ISubD mesh(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::ISubDSchema subSche = mesh.getSchema();
	}
	else if (Alembic::AbcGeom::IPolyMesh::matches(iObj.getHeader()))
	{
		
		//将其置为false，后面不会有相机的信息（感觉这个加不加没什么影响）
		hasOutputCamera=false;

		Alembic::AbcGeom::IPolyMesh mesh(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::IPolyMeshSchema meshSche = mesh.getSchema();
		//cout<<"IPolyMesh\n";
		//用来统计obj的数量
		++objCount;
		ss.clear();
		ss.str("");
		ss<<objCount;

		char tmp[40];
		time_t t=time(0);
		strftime( tmp, sizeof(tmp), "%Y-%m-%d %X",localtime(&t) );
		//尝试输出日志信息
		cout<<"objCount="<<ss.str()<<'\n';
        logFile<<tmp<<"\tanalysing..."<<"\t"<<ss.str()<<'\n';

		string add(tabNum, '\t'), 
			addObj = "<string name=\"filename\" value=\""  +string("a")+frameobj.str()+"_"+ss.str()+ ".obj\"/>";
		xmlContent.push_back(add + "<shape type=\"obj\">");
		
		//新添加的代码
		if(!addInXml.empty())
		{
			xmlContent.push_back(add + '\t' + "<transform name=\"toWorld\">");

			//动态获得vector的长度,这里只把obj的变换信息加入进去
			for(int i = addInXml.size() - 1; i >= 0; --i)
				xmlContent.push_back(add + "\t\t" + addInXml[i]);

			xmlContent.push_back(add + '\t' + "</transform>");
		}

		xmlContent.insert(xmlContent.end(), abcProp.begin(), abcProp.end());
		xmlContent.push_back(add + '\t' + addObj);
		xmlContent.push_back(add + "</shape>");
		xmlContent.push_back(string());

		//参考prman，尝试写polymesh的信息
		Alembic::AbcGeom::IPolyMeshSchema &ps = mesh.getSchema();
		TimeSamplingPtr ts = ps.getTimeSampling();


		SampleTimeSet sampleTimes;
		GetRelevantSampleTimes(ts, ps.getNumSamples(), sampleTimes);

		bool multisample = sampleTimes.size()>1;
		if (multisample)
		{
			WriteMotionBegin(sampleTimes);
		}
		//用来控制obj的数量
		int count=0;
		for (SampleTimeSet::iterator iter = sampleTimes.begin(); iter != sampleTimes.end(); ++iter)
		{
			ISampleSelector sampleSelector(*iter);
			IPolyMeshSchema::Sample sample = ps.getValue(sampleSelector);

			//获得点的信息
			P3fArraySamplePtr point = sample.getPositions();
			//打开文件的操作
			ofstream outfile;
			outfile.open((objPath + "a"+frameobj.str()+"_"+ss.str() + ".obj").c_str());

			//先进行强制类型转换，然后再获取输出内容
			float32_t *fpoint = (float32_t *)(point->getData());
			outfile << "v" << " ";
			int num = 0;
			for (int i = 0; i<3 * (point->size()); i++)
			{
				if (num != 0 && num % 3 == 0)
				{
					outfile << '\n';
					outfile << "v" << " ";
				}
				outfile << fixed << *fpoint << " ";
				fpoint++;
				num++;
			}
			outfile << '\n'; //这里如果需要endl，下面就不要加close，'\n'要和close一起，刷新缓冲区
			outfile.close();

			////尝试获得uv信息
			IV2fGeomParam uvParam = ps.getUVsParam();
			setUVs(frame, uvParam, objPath + "a"+frameobj.str()+"_"+ss.str() + ".obj");
			outfile.close();

			//尝试获得法向信息
			IN3fGeomParam nParam = ps.getNormalsParam();
			setPolyNormals(frame, nParam, objPath +"a"+frameobj.str()+"_"+ ss.str() + ".obj");
			outfile.close();

			//尝试获得面的索引信息
		    fillTopology(frame, uvParam, nParam, sample.getFaceIndices(), sample.getFaceCounts(), objPath + "a"+frameobj.str()+"_"+ ss.str() + ".obj");
		}
	}
	else if (Alembic::AbcGeom::ICamera::matches(iObj.getHeader()))
	{
		// chen
		// cout << "ICamera" << endl;
		Alembic::AbcGeom::ICamera cam(iObj, Alembic::Abc::kWrapExisting);

		Alembic::AbcGeom::ICameraSchema camSche = cam.getSchema();

		camSche = cam.getSchema();
		Alembic::AbcGeom::CameraSample *camsample = new Alembic::AbcGeom::CameraSample();//获取该模式的样本对象
		camsample = new Alembic::AbcGeom::CameraSample();
		camSche.get(*camsample);//获取样本数据

		//cout<<"camera\n";
		//cout<<iObj.getHeader().getFullName()<<'\n';
		//getCamera(frame,camSche);
	}
	else if (Alembic::AbcGeom::ICurves::matches(iObj.getHeader()))
	{
		// chen
		//cout << "ICurves" << endl;
		Alembic::AbcGeom::ICurves curves(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::ICurvesSchema curveSche = curves.getSchema();
		bool isConstant = curves.getSchema().isConstant();
		Alembic::AbcGeom::ICurvesSchema::Sample *cursamp = new Alembic::AbcGeom::ICurvesSchema::Sample();
		curveSche.get(*cursamp);
	}
	else if (Alembic::AbcGeom::INuPatch::matches(iObj.getHeader()))
	{
		// chen
		//cout << "INuPatch" << endl;
		Alembic::AbcGeom::INuPatch nurbs(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::INuPatchSchema inuSche = nurbs.getSchema();
	}
	else if (Alembic::AbcGeom::IPoints::matches(iObj.getHeader()))
	{
		// chen
		//cout << "IPoints" << endl;
		Alembic::AbcGeom::IPoints pts(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::IPointsSchema ipoSche = pts.getSchema();
	}
}