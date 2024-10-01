#include "BFEVFL.h"

#include "Logger.h"

template <typename T>
void BFEVFLFile::RadiaxTree<T>::Read(BFEVFLBinaryVectorReader& Reader)
{
	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic
	int32_t Count = Reader.ReadInt32();
	Reader.Seek(16, BinaryVectorReader::Position::Current);

	mStaticKeys.resize(Count);
	for (size_t i = 0; i < Count; i++)
	{
		Reader.Seek(8, BinaryVectorReader::Position::Current);
		mStaticKeys[i] = Reader.ReadStringPtr();
	}
}

template <typename T>
void BFEVFLFile::RadiaxTree<T>::LinkToArray(std::vector<T> Array)
{
	for (size_t i = 0; i < Array.size(); i++)
	{
		mItems.push_back(std::make_pair(mStaticKeys[i], Array[i]));
	}
}

void BFEVFLFile::ContainerData::ReadData(BFEVFLBinaryVectorReader& Reader, uint16_t Count, ContainerDataType Type)
{
	mType = Type;
	switch (Type)
	{
	case BFEVFLFile::ContainerDataType::Argument:
		mData = Reader.ReadStringPtr();
		break;
	case BFEVFLFile::ContainerDataType::Container:
	{
		RadiaxTree<ContainerItem>& Tree = std::get<RadiaxTree<ContainerItem>>(mData);
		std::vector<ContainerItem> Items;
		Items.resize(Count);
		Reader.ReadObjectOffsetsPtr<ContainerItem>(Items, [&Reader]()
			{
				ContainerItem Item;
				Item.Read(Reader);
				return Item;
			}, Reader.ReadUInt64());
		Tree.LinkToArray(Items);
		break;
	}
	case BFEVFLFile::ContainerDataType::Int:
		mData = Reader.ReadInt32();
		break;
	case BFEVFLFile::ContainerDataType::Bool:
		mData = Reader.ReadUInt32() == 0x80000001;
		break;
	case BFEVFLFile::ContainerDataType::Float:
		mData = Reader.ReadFloat();
		break;
	case BFEVFLFile::ContainerDataType::String:
		mData = Reader.ReadStringPtr();
		break;
	case BFEVFLFile::ContainerDataType::WString:
		mData = Reader.ReadStringPtr();
		break;
	case BFEVFLFile::ContainerDataType::IntArray:
	{
		std::vector<int32_t> Items;
		Items.resize(Count);
		for (size_t i = 0; i < Items.size(); i++)
		{
			Items[i] = Reader.ReadInt32();
		}
		mData = Items;
		break;
	}
	case BFEVFLFile::ContainerDataType::BoolArray:
	{
		std::deque<bool> Items;
		Items.resize(Count);
		for (size_t i = 0; i < Items.size(); i++)
		{
			Items[i] = Reader.ReadUInt32() == 0x80000001;
		}
		mData = Items;
		break;
	}
	case BFEVFLFile::ContainerDataType::FloatArray:
	{
		std::vector<float> Items;
		Items.resize(Count);
		for (size_t i = 0; i < Items.size(); i++)
		{
			Items[i] = Reader.ReadFloat();
		}
		mData = Items;
		break;
	}
	case BFEVFLFile::ContainerDataType::StringArray:
	{
		std::vector<std::string> Items;
		Items.resize(Count);
		for (size_t i = 0; i < Items.size(); i++)
		{
			Items[i] = Reader.ReadStringPtr();
		}
		mData = Items;
		break;
	}
	case BFEVFLFile::ContainerDataType::WStringArray:
	{
		std::vector<std::string> Items;
		Items.resize(Count);
		for (size_t i = 0; i < Items.size(); i++)
		{
			Items[i] = Reader.ReadStringPtr();
		}
		mData = Items;
		break;
	}
	case BFEVFLFile::ContainerDataType::ActorIdentifier:
	{
		std::string A = Reader.ReadStringPtr();
		std::string B = Reader.ReadStringPtr();
		mData = std::make_pair(A, B);
		break;
	}
	default:
		Logger::Error("ContainerData", "Unknown container data type");
		break;
	}
}

void BFEVFLFile::ContainerItem::Read(BFEVFLBinaryVectorReader& Reader)
{
	ContainerDataType DataType = static_cast<ContainerDataType>(Reader.ReadUInt8());
	Reader.Seek(1, BinaryVectorReader::Position::Current);

	mCount = Reader.ReadUInt16();
	Reader.Seek(4, BinaryVectorReader::Position::Current);

	uint64_t DictionaryOffset = Reader.ReadUInt64();
	if (DataType == ContainerDataType::Container)
	{
		mData = Reader.ReadObjectPtr<RadiaxTree<ContainerItem>>([&Reader]()
			{
				RadiaxTree<ContainerItem> Tree;
				Tree.Read(Reader);
				return Tree;
			}, DictionaryOffset);
	}
	
	if (!mIsRoot)
	{
		ReadData(Reader, mCount, DataType);
	}
}

void BFEVFLFile::Container::Read(BFEVFLBinaryVectorReader& Reader)
{
	ContainerItem Root;
	Root.mIsRoot = true;
	Root.Read(Reader);
	mStaticKeys = std::get<RadiaxTree<ContainerItem>>(Root.mData).mStaticKeys;
	std::vector<ContainerItem> Items;
	Items.resize(Root.mCount);
	Reader.ReadObjectOffsetsPtr<ContainerItem>(Items, [&Reader]()
		{
			ContainerItem Item;
			Item.Read(Reader);
			return Item;
		}, Reader.GetPosition());
	LinkToArray(Items);
}

void BFEVFLFile::Actor::Read(BFEVFLBinaryVectorReader& Reader)
{
	mName = Reader.ReadStringPtr();
	mSecondaryName = Reader.ReadStringPtr();
	mArgumentName = Reader.ReadStringPtr();

	int64_t ActionsOffset = Reader.ReadInt64();
	int16_t QueriesOffset = Reader.ReadInt64();
	mParameters = Reader.ReadObjectPtr<Container>([&Reader]()
		{
			Container Data;
			Data.Read(Reader);
			return Data;
		}, Reader.ReadUInt64());
	uint16_t mActionCount = Reader.ReadUInt16();
	uint16_t mQueryCount = Reader.ReadUInt16();

	mEntryPointIndex = Reader.ReadInt16();
	mCutNumber = Reader.ReadUInt8();

	Reader.Seek(1, BinaryVectorReader::Position::Current);

	mActions.resize(mActionCount);
	Reader.ReadObjectsPtr<std::string>(mActions, [&Reader]()
		{
			return Reader.ReadStringPtr();
		}, ActionsOffset);

	mQueries.resize(mQueryCount);
	Reader.ReadObjectsPtr<std::string>(mQueries, [&Reader]()
		{
			return Reader.ReadStringPtr();
		}, QueriesOffset);
}

std::pair<BFEVFLFile::EventType, std::variant<BFEVFLFile::ActionEvent, BFEVFLFile::SwitchEvent, BFEVFLFile::ForkEvent, BFEVFLFile::JoinEvent, BFEVFLFile::SubFlowEvent>> BFEVFLFile::Event::LoadTypeInstance(BFEVFLBinaryVectorReader& Reader)
{
	uint32_t Jumpback = Reader.GetPosition();
	Reader.Seek(8, BinaryVectorReader::Position::Current);
	EventType Type = (EventType)Reader.ReadUInt8();
	Reader.Seek(Jumpback, BinaryVectorReader::Position::Begin);
	
	switch (Type)
	{
	case BFEVFLFile::EventType::Action:
	{
		ActionEvent Action;
		Action.Read(Reader);
		return std::make_pair(Type, Action);
	}
	case BFEVFLFile::EventType::Switch:
	{
		SwitchEvent Switch;
		Switch.Read(Reader);
		return std::make_pair(Type, Switch);
	}
	case BFEVFLFile::EventType::Fork:
	{
		ForkEvent Fork;
		Fork.Read(Reader);
		return std::make_pair(Type, Fork);
	}
	case BFEVFLFile::EventType::Join:
	{
		JoinEvent Join;
		Join.Read(Reader);
		return std::make_pair(Type, Join);
	}
	case BFEVFLFile::EventType::SubFlow:
	{
		SubFlowEvent SubFlow;
		SubFlow.Read(Reader);
		return std::make_pair(Type, SubFlow);
	}
	}
}

void BFEVFLFile::Event::Read(BFEVFLBinaryVectorReader& Reader)
{
	mName = Reader.ReadStringPtr();
	mType = (EventType)Reader.ReadUInt8();
	Reader.Seek(1, BinaryVectorReader::Position::Current);
}

void BFEVFLFile::ActionEvent::Read(BFEVFLBinaryVectorReader& Reader)
{
	Event::Read(Reader);
	mNextEventIndex = Reader.ReadInt16();
	mActorIndex = Reader.ReadInt16();
	mActorActionIndex = Reader.ReadInt16();
	mParameters = Reader.ReadObjectPtr<Container>([&Reader]()
		{
			Container Data;
			Data.Read(Reader);
			return Data;
		}, Reader.ReadUInt64());
	Reader.Seek(16, BinaryVectorReader::Position::Current);
}

void BFEVFLFile::ForkEvent::Read(BFEVFLBinaryVectorReader& Reader)
{
	Event::Read(Reader);
	uint16_t ForkCount = Reader.ReadUInt16();
	mJoinEventIndex = Reader.ReadInt16();
	Reader.Seek(2, BinaryVectorReader::Position::Current);
	
	mForkEventIndicies.resize(ForkCount);
	Reader.ReadObjectsPtr<int16_t>(mForkEventIndicies, [&Reader]()
		{
			return Reader.ReadInt16();
		}, Reader.ReadUInt64());

	Reader.Seek(16, BinaryVectorReader::Position::Current);
}

void BFEVFLFile::JoinEvent::Read(BFEVFLBinaryVectorReader& Reader)
{
	Event::Read(Reader);
	mNextEventIndex = Reader.ReadInt16();
	Reader.Seek(28, BinaryVectorReader::Position::Current);
}

void BFEVFLFile::SubFlowEvent::Read(BFEVFLBinaryVectorReader& Reader)
{
	Event::Read(Reader);
	mNextEventIndex = Reader.ReadInt16();
	Reader.Seek(4, BinaryVectorReader::Position::Current);
	mParameters = Reader.ReadObjectPtr<Container>([&Reader]()
		{
			Container Data;
			Data.Read(Reader);
			return Data;
		}, Reader.ReadUInt64());
	mFlowchartName = Reader.ReadStringPtr();
	mEntryPointName = Reader.ReadStringPtr();
}

void BFEVFLFile::SwitchEvent::Read(BFEVFLBinaryVectorReader& Reader)
{
	Event::Read(Reader);
	uint16_t SwitchCaseCount = Reader.ReadUInt16();
	mActorIndex = Reader.ReadInt16();
	mActorQueryIndex = Reader.ReadInt16();
	mParameters = Reader.ReadObjectPtr<Container>([&Reader]()
		{
			Container Data;
			Data.Read(Reader);
			return Data;
		}, Reader.ReadUInt64());
	
	mSwitchCases.resize(SwitchCaseCount);
	Reader.ReadObjectsPtr<Case>(mSwitchCases, [&Reader]()
		{
			Case SwitchCase;
			SwitchCase.mValue = Reader.ReadInt32();
			SwitchCase.mEventIndex = Reader.ReadUInt16();
			Reader.ReadInt16();
			Reader.Align(8);
			return SwitchCase;
		}, Reader.ReadUInt64());
	Reader.Seek(8, BinaryVectorReader::Position::Current);
}

void BFEVFLFile::EntryPoint::Read(BFEVFLBinaryVectorReader& Reader)
{
	uint64_t SubFlowEventIndicesPtr = Reader.ReadUInt64();
	Reader.Seek(16, BinaryVectorReader::Position::Current);
	uint16_t SubFlowEventIndicesCount = Reader.ReadUInt16();
	Reader.Seek(2, BinaryVectorReader::Position::Current);
	mEventIndex = Reader.ReadInt16();
	Reader.Seek(2, BinaryVectorReader::Position::Current);
	mSubFlowEventIndices.resize(SubFlowEventIndicesCount);
	Reader.ReadObjectsPtr<int16_t>(mSubFlowEventIndices, [&Reader]()
		{
			return Reader.ReadInt16();
		}, SubFlowEventIndicesPtr);
}

void BFEVFLFile::Flowchart::Read(BFEVFLBinaryVectorReader& Reader)
{
	Reader.Seek(16, BinaryVectorReader::Position::Current);
	
	uint16_t ActorCount = Reader.ReadUInt16();
	
	Reader.Seek(4, BinaryVectorReader::Position::Current);
	
	uint16_t EventCount = Reader.ReadUInt16();
	uint16_t EntryPointCount = Reader.ReadUInt16();

	Reader.Seek(6, BinaryVectorReader::Position::Current);
	mName = Reader.ReadStringPtr();

	mActors.resize(ActorCount);
	Reader.ReadObjectsPtr<Actor>(mActors, [&Reader]()
		{
			Actor Obj;
			Obj.Read(Reader);
			return Obj;
		}, Reader.ReadUInt64());

	mEvents.resize(EventCount);
	Reader.ReadObjectsPtr<std::pair<BFEVFLFile::EventType, std::variant<BFEVFLFile::ActionEvent, BFEVFLFile::SwitchEvent, BFEVFLFile::ForkEvent, BFEVFLFile::JoinEvent, BFEVFLFile::SubFlowEvent>>>(mEvents, [&Reader]()
		{
			return Event::LoadTypeInstance(Reader);
		}, Reader.ReadUInt64());

	mEntryPoints = Reader.ReadObjectPtr<RadiaxTree<EntryPoint>>([&Reader]()
		{
			RadiaxTree<EntryPoint> Tree;
			Tree.Read(Reader);
			return Tree;
		}, Reader.ReadUInt64());
	std::vector<EntryPoint> EntryPoints;
	EntryPoints.resize(EntryPointCount);
	Reader.ReadObjectsPtr<EntryPoint>(EntryPoints, [&Reader]()
		{
			EntryPoint Point;
			Point.Read(Reader);
			return Point;
		}, Reader.ReadUInt64());
	mEntryPoints.LinkToArray(EntryPoints);
}

BFEVFLFile::BFEVFLFile(std::vector<unsigned char> Bytes)
{
	BFEVFLBinaryVectorReader Reader(Bytes);

	Reader.ReadStruct(&mHeader, sizeof(BFEVFLHeader));
	if (mHeader.mMagic[0] != 'B' || mHeader.mMagic[1] != 'F' || mHeader.mMagic[2] != 'E' || mHeader.mMagic[3] != 'V' || mHeader.mMagic[4] != 'F' || mHeader.mMagic[5] != 'L')
	{
		Logger::Error("BFEVFL", "Wrong magic, expected BFEVFL");
		return;
	}

	mFlowcharts.resize(mHeader.mFlowchartCount);
	Reader.ReadObjectOffsetsPtr<Flowchart>(mFlowcharts, [&Reader]() {
		Flowchart Flow;
		Flow.Read(Reader);
		return Flow;
		}, mHeader.mFlowchartOffsetsPtr);
	mLoaded = true;
	Logger::Info("BFEVFL", "Read event flow");
}