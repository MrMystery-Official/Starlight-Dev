#include "StaticCompoundImplementationBase.h"

namespace application::tool::scene
{
	void StaticCompoundImplementationBase::SyncBancEntityHashes(application::game::BancEntity& SceneEntity, uint64_t OldHash, uint64_t NewHash)
	{
		auto ReplaceHash = [](std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, uint64_t OldHash, uint64_t NewHash)
			{
				for (auto& [Key, Val] : Map)
				{
					if (std::holds_alternative<uint64_t>(Val) && std::get<uint64_t>(Val) == OldHash)
					{
						Val = NewHash;
					}
				}
			};

		if (SceneEntity.mHash == OldHash) SceneEntity.mHash = NewHash;

		ReplaceHash(SceneEntity.mDynamic, OldHash, NewHash);
		ReplaceHash(SceneEntity.mPresence, OldHash, NewHash);
		ReplaceHash(SceneEntity.mExternalParameter, OldHash, NewHash);

		ReplaceHash(SceneEntity.mPhive.mPlacement, OldHash, NewHash);

		if (SceneEntity.mPhive.mRopeHeadLink.has_value())
		{
			application::game::BancEntity::Phive::RopeLink& RopeLink = SceneEntity.mPhive.mRopeHeadLink.value();
			if (RopeLink.mID == OldHash) RopeLink.mID = NewHash;
			for (uint64_t& Owner : RopeLink.mOwners)
			{
				if (Owner == OldHash) Owner = NewHash;
			}
			for (uint64_t& Refer : RopeLink.mRefers)
			{
				if (Refer == OldHash) Refer = NewHash;
			}
		}

		if (SceneEntity.mPhive.mRopeTailLink.has_value())
		{
			application::game::BancEntity::Phive::RopeLink& RopeLink = SceneEntity.mPhive.mRopeTailLink.value();
			if (RopeLink.mID == OldHash) RopeLink.mID = NewHash;
			for (uint64_t& Owner : RopeLink.mOwners)
			{
				if (Owner == OldHash) Owner = NewHash;
			}
			for (uint64_t& Refer : RopeLink.mRefers)
			{
				if (Refer == OldHash) Refer = NewHash;
			}
		}

		if (SceneEntity.mPhive.mConstraintLink.has_value())
		{
			application::game::BancEntity::Phive::ConstraintLink& ConstraintLink = SceneEntity.mPhive.mConstraintLink.value();

			if (ConstraintLink.mID == OldHash) ConstraintLink.mID = NewHash;
			for (application::game::BancEntity::Phive::ConstraintLink::Refer& Refer : ConstraintLink.mRefers)
			{
				if (Refer.mOwner == OldHash) Refer.mOwner = NewHash;
			}

			for (application::game::BancEntity::Phive::ConstraintLink::Owner& Owner : ConstraintLink.mOwners)
			{
				ReplaceHash(Owner.mBreakableData, OldHash, NewHash);
				ReplaceHash(Owner.mAliasData, OldHash, NewHash);
				ReplaceHash(Owner.mClusterData, OldHash, NewHash);
				ReplaceHash(Owner.mParamData, OldHash, NewHash);
				ReplaceHash(Owner.mUserData, OldHash, NewHash);

				if (Owner.mRefer == OldHash) Owner.mRefer = NewHash;
			}
		}

		for (application::game::BancEntity::Link& Link : SceneEntity.mLinks)
		{
			if (Link.mDest == OldHash) Link.mDest = NewHash;
			if (Link.mSrc == OldHash) Link.mSrc = NewHash;
		}
	}

	bool StaticCompoundImplementationBase::Initialize()
	{
		return true;
	}

	bool StaticCompoundImplementationBase::Save()
	{
		return true;
	}

	void StaticCompoundImplementationBase::DrawStaticCompound(application::gl::Camera& Camera)
	{

	}

	uint64_t StaticCompoundImplementationBase::RequestUniqueHash()
	{
		return 0;
	}

	uint32_t StaticCompoundImplementationBase::RequestUniqueSRTHash()
	{
		return 0;
	}

	void StaticCompoundImplementationBase::OnBancEntityDelete(void* RenderInfo)
	{

	}

	void StaticCompoundImplementationBase::CleanupCollision(void* Scene)
	{

	}

	void StaticCompoundImplementationBase::CleanupCollisionVolume(void* Scene)
	{

	}
}