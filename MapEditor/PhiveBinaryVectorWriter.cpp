#include "PhiveBinaryVectorWriter.h"

#include "Util.h"
#include "Logger.h"

void PhiveBinaryVectorWriter::QueryHkArrayWrite(std::string Name)
{
	Align(8);
	mArrayOffsets.insert({ Name, GetPosition() });
	Seek(16, BinaryVectorWriter::Position::Current);
}

void PhiveBinaryVectorWriter::QueryHkArrayToItemPointerWrite(std::string Name)
{
	Align(8);
	mArrayOffsets.insert({ Name, GetPosition() });
	Seek(8, BinaryVectorWriter::Position::Current);
}

void PhiveBinaryVectorWriter::QueryHkPointerWrite(std::string Name)
{
	Align(8);
	mPointerOffsets.insert({ Name, GetPosition() });
	Seek(8, BinaryVectorWriter::Position::Current);
}

PhiveBinaryVectorWriter::PhiveBinaryVectorWriter(PhiveWrapper& mWrapper) : mPhiveWrapper(mWrapper)
{
	mPhiveWrapper.mItems.resize(2); //Keep first two elements
}