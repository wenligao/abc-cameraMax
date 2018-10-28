//analyseXml.h
#ifndef _ANALYSEXML_H

#define _ANALYSEXML_H


#include "traverse.h"
#include "global.h"
#include <fstream>
#include <time.h>
using namespace std;

#include <streambuf>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcGeom/Foundation.h>
using namespace Alembic::AbcGeom;



//下面是函数的声明
void walk(Alembic::Abc::IArchive & iRoot);
void subdivide(const string &filePath) ;
void xmlAnalyze(const string &xmlFile);





#endif 