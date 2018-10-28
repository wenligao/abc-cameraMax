#include <fstream>
#include <iostream>
#include <time.h>
#include "global.h"
#include "animatePorcess.h"
#include "SampleUtil.h"
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
		//cout<<"IXform\n";
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
				//cout<<"multiSample为True"<<'\n';
			}
			for (size_t j = 0; j < sampleVectors.size(); ++j)
			{
				XformOp &op = sampleVectors[j][i];
				switch (op.getType())
				{
				case kScaleOperation:
					{

						V3d value = op.getScale();
						// chen
						ostringstream oss;
						oss << "<scale x=\"" << value.x << "\" y=\""<< value.y << "\" z=\"" << value.z <<"\"/>";
						cout << "<scale x=\"" << value.x << "\" y=\""<< value.y << "\" z=\"" << value.z <<"\"/>"<<'\n';
						addInXml.push_back(oss.str());
					}
					break;

				case kTranslateOperation:
					{
						V3d value = op.getTranslate();
						// chen
						ostringstream oss;
						oss << "<translate x=\"" << value.x << "\" y=\""<< value.y << "\" z=\"" << value.z <<"\"/>";
						cout << "<translate x=\"" << value.x << "\" y=\""<< value.y << "\" z=\"" << value.z <<"\"/>"<<'\n';
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
						cout << "<rotate x=\"" << axis.x << "\" angle=\""<< degrees << "\"/>"<<'\n';
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
						cout << "<rotate y=\"" << axis.y << "\" angle=\""<< degrees << "\"/>"<<'\n';
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
						cout << "<rotate z=\"" << axis.z << "\" angle=\""<< degrees << "\"/>"<<'\n';
						addInXml.push_back(oss.str());
					}
					break;
					
				/*case kMatrixOperation:
					{
						Alembic::Abc::M44d os_mat = op.getMatrix();
						for (int i = 0; i<4; i++)
						{
							for (int j = 0; j<4; j++)
							{
								cout << os_mat[i][j] << " ";
							}
							cout << '\n';
						}
					}
					break;*/
				}//switch语句结束
			}
		}
		//xformfile.close();
		size_t numChildren = iObject->getNumChildren();
		for (size_t i = 0; i<numChildren; i++)
		{
			visit(iObject->getChild(i));
			
		}
	}
	//循环遍历场景中的节点
	else if (Alembic::AbcGeom::ISubD::matches(iObj.getHeader()))
	{
		Alembic::AbcGeom::ISubD mesh(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::ISubDSchema subSche = mesh.getSchema();
		cout<<"subd\n";

	}
	else if (Alembic::AbcGeom::IPolyMesh::matches(iObj.getHeader()))
	{
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

		/*string add(tabNum, '\t'), 
			addObj = "<string name=\"filename\" value=\"" + objPath +"a"+frameobj.str()+"_"+ss.str()+ ".obj\"/>";*/
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

		//xmlContent.push_back(add + '\t' + "<transform name=\"toWorld\">");

		////动态获得vector的长度
		//for(int i = addInXml.size() - 1; i >= 0; --i)
		//	xmlContent.push_back(add + "\t\t" + addInXml[i]);

		//xmlContent.push_back(add + '\t' + "</transform>");

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
			//outfile.open(("a"+frameobj.str()+"_"+ss.str() + ".obj").c_str());

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
			outfile << '\n'; //这里必须使用endl,否则在输出vt的信息时会报错
			//outfile << "lucky"<<'\n';
			outfile.close();

			////尝试获得uv信息
			IV2fGeomParam uvParam = ps.getUVsParam();
			setUVs(frame, uvParam, objPath + "a"+frameobj.str()+"_"+ss.str() + ".obj");
			//setUVs(frame, uvParam, "a"+frameobj.str()+"_"+ss.str() + ".obj");
			outfile.close();

			//尝试获得法向信息
			IN3fGeomParam nParam = ps.getNormalsParam();
			setPolyNormals(frame, nParam, objPath +"a"+frameobj.str()+"_"+ ss.str() + ".obj");
			//setPolyNormals(frame, nParam,"a"+frameobj.str()+"_"+ ss.str() + ".obj");
			outfile.close();

			//尝试获得面的索引信息
		    fillTopology(frame, uvParam, nParam, sample.getFaceIndices(), sample.getFaceCounts(), objPath + "a"+frameobj.str()+"_"+ ss.str() + ".obj");
			//fillTopology(frame, uvParam, nParam, sample.getFaceIndices(), sample.getFaceCounts(),"a"+frameobj.str()+"_"+ ss.str() + ".obj");

			cout<<"polymesh\n";
			//cout<<iObj.getHeader().getFullName()<<'\n';
		}//for循环结束

	}//if循环结束
	else if (Alembic::AbcGeom::ICamera::matches(iObj.getHeader()))
	{
		Alembic::AbcGeom::ICamera cam(iObj, Alembic::Abc::kWrapExisting);

		Alembic::AbcGeom::ICameraSchema camSche = cam.getSchema();
		//camSche = cam.getSchema();
		//Alembic::AbcGeom::CameraSample *camsample = new Alembic::AbcGeom::CameraSample();//获取该模式的样本对象
		//camsample = new Alembic::AbcGeom::CameraSample();
		//camSche.get(*camsample);//获取样本数据

		cout<<"camera\n";
		//cout<<iObj.getHeader().getFullName()<<'\n';
		//getCamera(frame,camSche);
	}
	else if (Alembic::AbcGeom::ICurves::matches(iObj.getHeader()))
	{
		Alembic::AbcGeom::ICurves curves(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::ICurvesSchema curveSche = curves.getSchema();
		bool isConstant = curves.getSchema().isConstant();
		Alembic::AbcGeom::ICurvesSchema::Sample *cursamp = new Alembic::AbcGeom::ICurvesSchema::Sample();
		curveSche.get(*cursamp);
		cout<<"curves\n";
	}
	else if (Alembic::AbcGeom::INuPatch::matches(iObj.getHeader()))
	{
		Alembic::AbcGeom::INuPatch nurbs(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::INuPatchSchema inuSche = nurbs.getSchema();
		cout<<"nupatch\n";

	}
	else if (Alembic::AbcGeom::IPoints::matches(iObj.getHeader()))
	{
		Alembic::AbcGeom::IPoints pts(iObj, Alembic::Abc::kWrapExisting);
		Alembic::AbcGeom::IPointsSchema ipoSche = pts.getSchema();
		cout<<"points\n";
	}
	else if(Alembic::AbcGeom::IFaceSet::matches(iObj.getHeader()))//new add
	{
		cout<<"faceset\n";
	}
	else if(Alembic::AbcGeom::ILight::matches(iObj.getHeader()))
	{
		cout<<"light\n";
	}
	else if(Alembic::AbcGeom::IGeomBaseObject::matches(iObj.getHeader()))
	{
		cout<<"geom\n";
	}
	else 
	{
		cout<<"no mataches\n";
	}
}