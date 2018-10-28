#ifndef _SAMPLEUTIL_H

#define _SAMPLE_HUTIL_H


#include <Alembic/AbcGeom/All.h>
using namespace Alembic::AbcGeom;

typedef std::set<Abc::chrono_t> SampleTimeSet;

void WriteMotionBegin(const SampleTimeSet &sampleTimes);


#endif