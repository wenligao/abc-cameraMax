#ifndef _ANIMATEPORCESS_H

#define _ANIMATEPORCESS_H

#include <Alembic/AbcGeom/All.h>
#include "traverse.h"
#include <set>

using namespace Alembic::AbcGeom;


template <typename T>
T simpleLerp(double alpha, T val1, T val2)
{
	double dv=static_cast<double>(val1);
	return static_cast<T> (dv + alpha * (static_cast<double>(val2) - dv) );
}

struct Prop
{
	Alembic::Abc::IArrayProperty mArray;
	Alembic::Abc::IScalarProperty mScalar;
};
typedef std::set<Abc::chrono_t> SampleTimeSet;

void GetRelevantSampleTimes(TimeSamplingPtr timeSampling,
	size_t numSamples, SampleTimeSet &output);

double getWeightAndIndex(double iFrame,
	Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps,
	Alembic::AbcCoreAbstract::index_t & oIndex,
	Alembic::AbcCoreAbstract::index_t & oCeilIndex);

void setUVs(double iFrame, Alembic::AbcGeom::IV2fGeomParam iUVs, string filename);

void setPolyNormals(double iFrame, Alembic::AbcGeom::IN3fGeomParam iNormals, string filename);

void fillTopology(
	double iFrame, Alembic::AbcGeom::IV2fGeomParam iUVs,
	Alembic::AbcGeom::IN3fGeomParam iNormals,
	Alembic::Abc::Int32ArraySamplePtr iIndices,
	Alembic::Abc::Int32ArraySamplePtr iCounts, string filename);


void getCamera(double iFrame,Alembic::AbcGeom::ICameraSchema camSche);


#endif