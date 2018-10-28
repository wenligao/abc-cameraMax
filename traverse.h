#ifndef _TRAVERSE_H

#define _TRAVERSE_H

#include <Alembic/AbcGeom/ICamera.h>
#include <Alembic/AbcGeom/ICurves.h>
#include <Alembic/AbcGeom/IPoints.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/ISubD.h>
#include <Alembic/AbcGeom/IXform.h>
#include <Alembic/AbcGeom/INuPatch.h>

class AlembicObject;
typedef Alembic::Util::shared_ptr<AlembicObject> AlembicObjectPtr;
class AlembicObject
{
public:
	AlembicObject(const Alembic::Abc::IObject& iObject) : mObject(iObject) {}

	const   Alembic::Abc::IObject&  object() const { return mObject; }
	Alembic::Abc::IObject&  object()       { return mObject; }

	void    addChild(AlembicObjectPtr iChild)   { mChildren.push_back(iChild); }
	size_t  getNumChildren()                    { return mChildren.size(); }

	AlembicObjectPtr      getChild(size_t index){ return mChildren[index]; }
private:
	Alembic::Abc::IObject       mObject;
	std::vector<AlembicObjectPtr> mChildren;
};

AlembicObjectPtr previsit(AlembicObjectPtr iParentObj);
void visit(AlembicObjectPtr iObject);
void writeSensor();
//void writeSensorMax();

#endif