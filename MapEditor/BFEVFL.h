#pragma once

#include <vector>
#include <string>
#include <variant>
#include <deque>
#include "BFEVFLBinaryVectorReader.h"

class BFEVFLFile
{
public:
	struct BFEVFLHeader
	{
		char mMagic[6];
		char mPadding0[2];
		uint8_t mVersionA;
		uint8_t mVersionB;
		uint8_t mVersionC;
		uint8_t mVersionD;
		char mPadding1[4];
		uint32_t mFileNameOffset;
		char mPadding2[12];
		uint16_t mFlowchartCount;
		uint16_t mTimelineCount;
		char mPadding3[4];
		uint64_t mFlowchartOffsetsPtr;
		uint64_t mFlowchartNameDictionaryOffset;
		uint64_t mTimelineOffsetsPtr;
		uint64_t mTimelineNameDictionaryOffset;
	};

	template <typename T>
	struct RadiaxTree
	{
		std::vector<std::string> mStaticKeys;
		std::vector<std::pair<std::string, T>> mItems;

		void LinkToArray(std::vector<T> Array);
		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	enum class ContainerDataType : uint8_t
	{
		Argument = 0,
		Container = 1,
		Int = 2,
		Bool = 3,
		Float = 4,
		String = 5,
		WString = 6,
		IntArray = 7,
		BoolArray = 8,
		FloatArray = 9,
		StringArray = 10,
		WStringArray = 11,
		ActorIdentifier = 12
	};

	struct ContainerItem;
	typedef struct ContainerItem ContainerItem;

	struct ContainerData
	{
		std::variant<std::string, RadiaxTree<ContainerItem>, int32_t, bool, float, std::vector<int32_t>, std::deque<bool>, std::vector<float>, std::vector<std::string>, std::pair<std::string, std::string>> mData;
		ContainerDataType mType;

		void ReadData(BFEVFLBinaryVectorReader& Reader, uint16_t Count, ContainerDataType Type);
	};

	struct ContainerItem : public ContainerData
	{
		bool mIsRoot = false;
		uint16_t mCount;

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct Container : public RadiaxTree<ContainerItem>
	{
		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct Actor
	{
		std::string mName;
		std::string mSecondaryName;
		std::string mArgumentName;
		uint16_t mEntryPointIndex;
		uint8_t mCutNumber;
		std::vector<std::string> mActions;
		std::vector<std::string> mQueries;
		Container mParameters;

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	enum class EventType : uint8_t
	{
		Action = 0,
		Switch = 1,
		Fork = 2,
		Join = 3,
		SubFlow = 4
	};

	struct ActionEvent;
	typedef struct ActionEvent ActionEvent;
	struct SwitchEvent;
	typedef struct SwitchEvent SwitchEvent;
	struct ForkEvent;
	typedef struct ForkEvent ForkEvent;
	struct JoinEvent;
	typedef struct JoinEvent JoinEvent;
	struct SubFlowEvent;
	typedef struct SubFlowEvent SubFlowEvent;

	struct Event
	{
		std::string mName;
		EventType mType;

		static std::pair<EventType, std::variant<ActionEvent, SwitchEvent, ForkEvent, JoinEvent, SubFlowEvent>> LoadTypeInstance(BFEVFLBinaryVectorReader& Reader);
		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct ActionEvent : public Event
	{
		int16_t mNextEventIndex = -1;
		int16_t mActorIndex = -1;
		int16_t mActorActionIndex = -1;
		Container mParameters;

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct ForkEvent : public Event
	{
		int16_t mJoinEventIndex = -1;
		std::vector<int16_t> mForkEventIndicies;

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct JoinEvent : public Event
	{
		int16_t mNextEventIndex = -1;

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct SubFlowEvent : public Event
	{
		int16_t mNextEventIndex = -1;
		Container mParameters;
		std::string mFlowchartName = "";
		std::string mEntryPointName = "";

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct SwitchEvent : public Event
	{
		struct Case
		{
			int32_t mValue = -1;
			int16_t mEventIndex = -1;
		};

		int16_t mActorIndex = -1;
		int16_t mActorQueryIndex = -1;
		Container mParameters;
		std::vector<Case> mSwitchCases;

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct EntryPoint
	{
		std::vector<int16_t> mSubFlowEventIndices;
		int16_t mEventIndex = -1;

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	struct Flowchart
	{
		std::string mName;
		std::vector<Actor> mActors;
		std::vector<std::pair<EventType, std::variant<ActionEvent, SwitchEvent, ForkEvent, JoinEvent, SubFlowEvent>>> mEvents;
		RadiaxTree<EntryPoint> mEntryPoints;

		void Read(BFEVFLBinaryVectorReader& Reader);
	};

	BFEVFLHeader mHeader;
	std::vector<Flowchart> mFlowcharts;
	bool mLoaded = false;

	BFEVFLFile(std::vector<unsigned char> Bytes);
	BFEVFLFile() = default;
};