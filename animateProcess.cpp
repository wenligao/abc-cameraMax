#include <fstream>
#include "animatePorcess.h"
#include "global.h"
#include <Alembic/AbcGeom/All.h>

using namespace std;

//参考prman
void GetRelevantSampleTimes(TimeSamplingPtr timeSampling,
	size_t numSamples, SampleTimeSet &output)
{
	if (numSamples<2)
	{
		output.insert(0.0);
		return;
	}
	chrono_t frameTime = frame / 24.0;//相当于这一帧所在的时间
	chrono_t shutterOpenTime = (frame + 0.0) / 24.0;
	chrono_t shutterCloseTime = (frame + 0.5) / 24.0;
	std::pair<index_t, chrono_t> shutterOpenFloor = timeSampling->getFloorIndex(shutterOpenTime, numSamples);//取下整
	std::pair<index_t, chrono_t> shutterCloseCeil = timeSampling->getCeilIndex(shutterCloseTime, numSamples);//取上整


	for (index_t i = shutterOpenFloor.first; i<shutterCloseCeil.first; ++i)
	{
		output.insert(timeSampling->getSampleTime(i));//获得任何样本的时间
	}

	if (output.size() == 0)
	{
		output.insert(frameTime);
		return;
	}

}

double getWeightAndIndex(double iFrame,
	Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps,
	Alembic::AbcCoreAbstract::index_t & oIndex,
	Alembic::AbcCoreAbstract::index_t & oCeilIndex)
{
	if (numSamps == 0)
		numSamps = 1;

	//找到时间小于或等于给定时间的最大有效索引，使用零个样本无效
	//如果最小采样时间大于iFrame，则返回0
	std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
		iTime->getFloorIndex(iFrame, numSamps);

	oIndex = floorIndex.first;
	oCeilIndex = oIndex;

	if (fabs(iFrame - floorIndex.second) < 0.0001)
		return 0.0;

	std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
		iTime->getCeilIndex(iFrame, numSamps);

	if (oIndex == ceilIndex.first)
		return 0.0;

	oCeilIndex = ceilIndex.first;

	double alpha = (iFrame - floorIndex.second) /
		(ceilIndex.second - floorIndex.second);

	// we so closely match the ceiling so we'll just use it
	if (fabs(1.0 - alpha) < 0.0001)
	{
		oIndex = oCeilIndex;
		return 0.0;
	}

	return alpha;
}

void setUVs(double iFrame, Alembic::AbcGeom::IV2fGeomParam iUVs, string filename)
{
	ofstream outfile;
	outfile.open(filename.c_str(), ios::app);
	if (!iUVs.valid())
	{
		//cout << "uv无效" << '\n';
		return;
	}
	Alembic::AbcCoreAbstract::index_t index, ceilIndex;
	getWeightAndIndex(iFrame, iUVs.getTimeSampling(), iUVs.getNumSamples(), index, ceilIndex);
	Alembic::AbcGeom::IV2fGeomParam::Sample samp;
	iUVs.getIndexed(samp, Alembic::Abc::ISampleSelector(index));

	Alembic::AbcGeom::V2fArraySamplePtr uvPtr = samp.getVals();
	Alembic::Abc::UInt32ArraySamplePtr indexPtr = samp.getIndices();
	unsigned int numUVs = (unsigned int)uvPtr->size();
	for (unsigned int i = 0; i < numUVs; ++i)
	{
		outfile << "vt " << fixed << (*uvPtr)[i].x << " "<<fixed<<(*uvPtr)[i].y<< '\n' ;//ww
		//outfile << "vt " << fixed << (*uvPtr)[i].y <<endl;
	}
}

void setPolyNormals(double iFrame, Alembic::AbcGeom::IN3fGeomParam iNormals, string filename)
{
	ofstream outfile;
	outfile.open(filename.c_str(), ios::app);
	if (!iNormals)
	{
		//cout << "normal无效！" << endl;
		return;
	}

	if (iNormals.getScope() != Alembic::AbcGeom::kVertexScope &&
		iNormals.getScope() != Alembic::AbcGeom::kVaryingScope &&
		iNormals.getScope() != Alembic::AbcGeom::kFacevaryingScope)
	{
		return;
	}

	Alembic::AbcCoreAbstract::index_t index, ceilIndex;
	double alpha = getWeightAndIndex(iFrame,
		iNormals.getTimeSampling(), iNormals.getNumSamples(),
		index, ceilIndex);
	Alembic::AbcGeom::IN3fGeomParam::Sample samp;
	iNormals.getExpanded(samp, Alembic::Abc::ISampleSelector(index));

	Alembic::Abc::N3fArraySamplePtr sampVal = samp.getVals();

	size_t sampSize = sampVal->size();

	Alembic::Abc::N3fArraySamplePtr ceilVals;
	if (alpha != 0 && index != ceilIndex)
	{
		Alembic::AbcGeom::IN3fGeomParam::Sample ceilSamp;
		iNormals.getExpanded(ceilSamp, Alembic::Abc::ISampleSelector(ceilIndex));
		ceilVals = ceilSamp.getVals();
		if (sampSize == ceilVals->size())
		{
			Alembic::Abc::N3fArraySamplePtr ceilVal = ceilSamp.getVals();
			for (size_t i = 0; i<sampSize; ++i)
			{
				//cout << "待扩充！" << endl;
			}
		}
		else
		{
			for (size_t i = 0; i < sampSize; ++i)
			{
				outfile << "vn " << fixed << (*sampVal)[i].x << " " << (*sampVal)[i].y << " " << (*sampVal)[i].z << '\n';
			}
		}
	}
	else
	{
		for (size_t i = 0; i < sampSize; ++i)
		{
			outfile << "vn " << fixed << (*sampVal)[i].x << " " << (*sampVal)[i].y << " " << (*sampVal)[i].z << '\n';
		}
	}

}

void fillTopology(
	double iFrame,
	Alembic::AbcGeom::IV2fGeomParam iUVs,
	Alembic::AbcGeom::IN3fGeomParam iNormals,
	Alembic::Abc::Int32ArraySamplePtr iIndices,
	Alembic::Abc::Int32ArraySamplePtr iCounts, string filename)
{
	ofstream outfile;
	outfile.open(filename.c_str(), ios::app);
	unsigned int numPolys = static_cast<unsigned int>(iCounts->size());
	int *polyCounts = new int[numPolys];

	for (unsigned int i = 0; i<numPolys; i++)
	{
		polyCounts[i] = (*iCounts)[i];//每一行都是4个点的信息
	}

	unsigned int numConnects = static_cast<unsigned int>(iIndices->size());

	int *polyConnects = new int[numConnects];

	unsigned int facePointIndex = 0;
	unsigned int base = 0;
	Alembic::Abc::UInt32ArraySamplePtr indexPtr;
	if (iUVs)
	{
		//尝试输出uv索引
		//cout<<"uv index\n";
		Alembic::AbcCoreAbstract::index_t index, ceilIndex;
		getWeightAndIndex(iFrame, iUVs.getTimeSampling(), iUVs.getNumSamples(), index, ceilIndex);
		Alembic::AbcGeom::IV2fGeomParam::Sample samp;
		iUVs.getIndexed(samp, Alembic::Abc::ISampleSelector(index));
		Alembic::AbcGeom::V2fArraySamplePtr uvPtr = samp.getVals();
		indexPtr = samp.getIndices();
	}

	Alembic::Abc::UInt32ArraySamplePtr norPtr;
	if (iNormals)
	{
		//尝试输出法向索引
		//cout<<"normal index\n";
		Alembic::AbcCoreAbstract::index_t indexnor, ceilIndexnor;
		double alpha = getWeightAndIndex(iFrame,
			iNormals.getTimeSampling(), iNormals.getNumSamples(),
			indexnor, ceilIndexnor);
		/*if (alpha != 0)
			cout << "继续执行该程序" << '\n';*/
		Alembic::AbcGeom::IN3fGeomParam::Sample sampnor;


		iNormals.getIndexed(sampnor, Alembic::Abc::ISampleSelector(indexnor));

		Alembic::Abc::N3fArraySamplePtr sampVal = sampnor.getVals();
		norPtr = sampnor.getIndices();
	}

	int uvIndex = 0;
	int normalIndex = 0;

	for (unsigned int i = 0; i<numPolys; i++)
	{
		int curNum = polyCounts[i];
		outfile << "f" << " ";

		if (curNum == 0)
			continue;

		int normal = normalIndex + curNum - 1;

		int startpoint = uvIndex + curNum - 1;
		int nor = curNum - 1;
		for (int j = 0; j < curNum; ++j)
		{
			outfile << (*iIndices)[base + curNum - j - 1] + 1;//点的索引
			//有uv无法向
			if (iUVs && !iNormals)
			{
				outfile << "/";
				outfile << (*indexPtr)[startpoint - j] + 1 << " ";
			}
			//有法向无uv
			if (!iUVs && iNormals)
			{
				outfile << "//" << (*norPtr)[startpoint - j] + 1 << " ";
			}
			//有uv有法向
			if (iUVs && iNormals)
			{
				outfile << "/" << (*indexPtr)[startpoint - j] + 1;
				//outfile<<"/"<<(*norPtr)[normal-nor]+1<<" ";
				outfile << "/" << (*norPtr)[startpoint - j] + 1 << " ";

			}
			//无法向无uv
			if (!iUVs && !iNormals)
			{
				outfile << " ";
			}
			uvIndex++;
			nor--;
			normalIndex++;

		}
//www
		outfile << '\n';
		base += curNum;
	}
}



//用来获得相机的参数
void getCamera(double iFrame,Alembic::AbcGeom::ICameraSchema camSche)
{
	//参考abcImport中的部分写相机
	 
		vector<double> oArray;
		oArray.resize(18);

		//设置一些可选的比例值
		oArray[13]=1.0;
		oArray[16] = 1.0;
		oArray[17] = 1.0;

		Alembic::AbcCoreAbstract::index_t index,ceilIndex;
		double alpha=getWeightAndIndex(frame,camSche.getTimeSampling(),camSche.getNumSamples(),index,ceilIndex);

		if(alpha !=0.0)
		{
			Alembic::AbcGeom::CameraSample samp,ceilSamp;
			camSche.get(samp,index);//重置一些相机参数
			camSche.get(ceilSamp,ceilIndex);

			oArray[0] = simpleLerp<double>(alpha, samp.getFocalLength(),
				ceilSamp.getFocalLength());
		    oArray[1] = simpleLerp<double>(alpha, samp.getLensSqueezeRatio(),
				ceilSamp.getLensSqueezeRatio());
            oArray[2] = simpleLerp<double>(alpha, samp.getHorizontalAperture(),
				ceilSamp.getHorizontalAperture()) / 2.54;
			oArray[3] = simpleLerp<double>(alpha, samp.getVerticalAperture(),
				ceilSamp.getVerticalAperture()) / 2.54;
			oArray[4] = simpleLerp<double>(alpha,
				samp.getHorizontalFilmOffset(),
				ceilSamp.getHorizontalFilmOffset()) / 2.54;
			oArray[5] = simpleLerp<double>(alpha,
				samp.getVerticalFilmOffset(),
				ceilSamp.getVerticalFilmOffset()) / 2.54;

			if (samp.getOverScanLeft() == samp.getOverScanRight() &&
				samp.getOverScanTop() == samp.getOverScanBottom() &&
				samp.getOverScanLeft() == samp.getOverScanTop() &&
				ceilSamp.getOverScanLeft() == ceilSamp.getOverScanRight() &&
				ceilSamp.getOverScanTop() == ceilSamp.getOverScanBottom() &&
				ceilSamp.getOverScanLeft() == ceilSamp.getOverScanTop())
			{
				oArray[6] = simpleLerp<double>(alpha,
					samp.getOverScanLeft() + 1.0,
					ceilSamp.getOverScanLeft() + 1.0);
			}
			else
			{
				oArray[6] = 1.0;
			}

			oArray[7] = simpleLerp<double>(alpha, samp.getNearClippingPlane(),
				ceilSamp.getNearClippingPlane());

			oArray[8] = simpleLerp<double>(alpha, samp.getFarClippingPlane(),
				ceilSamp.getFarClippingPlane());

			oArray[9] = simpleLerp<double>(alpha, samp.getFStop(),
				ceilSamp.getFStop());

			oArray[10] = simpleLerp<double>(alpha, samp.getFocusDistance(),
				ceilSamp.getFocusDistance());

			double shutterClose = simpleLerp<double>(alpha, samp.getShutterClose(),
				ceilSamp.getShutterClose());
			double shutterOpen = simpleLerp<double>(alpha, samp.getShutterOpen(),
				ceilSamp.getShutterOpen());

			std::size_t numOps = samp.getNumOps();
			for (std::size_t i = 0; i < numOps; ++i)
			{
				Alembic::AbcGeom::FilmBackXformOp & op = samp[i];
				Alembic::AbcGeom::FilmBackXformOp & ceilOp = ceilSamp[i];
				if (op.getHint() == "filmFitOffs")
				{
					double val = op.getChannelValue(0) *
						samp.getHorizontalAperture();

					double ceilVal = ceilOp.getChannelValue(0) *
						ceilSamp.getHorizontalAperture();

					if (val != 0.0)
					{
						// chanValue(0) * 0.5 * horiz aper / 2.54
						oArray[12] = simpleLerp<double>(alpha, val, ceilVal) / 5.08;
					}
					else
					{
						val = op.getChannelValue(1) * samp.getHorizontalAperture();

						ceilVal = ceilOp.getChannelValue(1) *
							ceilSamp.getHorizontalAperture();

						// chanValue(1)* 0.5 * horiz aper / 2.54
						oArray[12] = simpleLerp<double>(alpha, val, ceilVal) / 5.08;
					}
				}
				//获取帮助消除某些可能具有相同类型的选项的暗示
				else if (op.getHint() == "preScale")
				{
					oArray[13] = 1.0 / simpleLerp<double>(alpha,
						op.getChannelValue(0), ceilOp.getChannelValue(0));
				}
				else if (op.getHint() == "filmTranslate")
				{
					oArray[14] = simpleLerp<double>(alpha,
						op.getChannelValue(0), ceilOp.getChannelValue(0));

					oArray[15] = simpleLerp<double>(alpha,
						op.getChannelValue(1), ceilOp.getChannelValue(1));
				}
				else if (op.getHint() == "postScale")
				{
					oArray[16] = 1.0 / simpleLerp<double>(alpha,
						op.getChannelValue(0), ceilOp.getChannelValue(0));
				}
				else if (op.getHint() == "cameraScale")
				{
					oArray[17] = simpleLerp<double>(alpha,
						op.getChannelValue(0), ceilOp.getChannelValue(0));
				}
			}

		}else
		{
			Alembic::AbcGeom::CameraSample samp;
			camSche.get(samp,index);
			oArray[0] = samp.getFocalLength();
			oArray[1] = samp.getLensSqueezeRatio();
			oArray[2] = samp.getHorizontalAperture()/2.54;
			oArray[3] = samp.getVerticalAperture()/2.54;
			oArray[4] = samp.getHorizontalFilmOffset()/2.54;
			oArray[5] = samp.getVerticalFilmOffset()/2.54;

			if(samp.getOverScanLeft() == samp.getOverScanRight() &&
				samp.getOverScanTop() == samp.getOverScanBottom() &&
				samp.getOverScanLeft() == samp.getOverScanTop())
			{
				oArray[6] = samp.getOverScanLeft() + 1.0;
			}
			else
			{
				oArray[6] = 1.0;
			}
		
			oArray[7] = samp.getNearClippingPlane();
			oArray[8] = samp.getFarClippingPlane();

			oArray[9] = samp.getFStop();
			oArray[10] = samp.getFocusDistance();

			size_t numOps = samp.getNumOps();
			for(size_t i = 0; i< numOps; ++i)
			{
				Alembic::AbcGeom::FilmBackXformOp & op =samp[i];
				if(op.getHint() == "filmFitOffs")
				{
					if(op.getChannelValue(0) != 0.0)
					{
						oArray[12] =op.getChannelValue(0) * 
							samp.getHorizontalAperture() / 5.08;
					}
					else
					{
						oArray[12] = op.getChannelValue(1) *
							samp.getHorizontalAperture() /5.08;
					}
				}
				else if(op.getHint() =="preScale")
				{
					oArray[13] =1.0 / op.getChannelValue(0);
				}
				else if(op.getHint() =="filmTranslate")
				{
					oArray[14] = op.getChannelValue(0);
					oArray[15] = op.getChannelValue(1);
				}
				else if (op.getHint() == "postScale")
				{
					oArray[16] = 1.0 / op.getChannelValue(0);
				}
				else if (op.getHint() == "cameraScale")
				{
					oArray[17] = op.getChannelValue(0);
				}
			}
		}//最外层对应的else函数


	/*	cout<<"输出相机对应的参数\n";
		for(size_t i = 0;i < oArray.size(); i++)
			cout<<oArray[i]<<" ";
		cout<<'\n';*/

}//getCamera函数结束
