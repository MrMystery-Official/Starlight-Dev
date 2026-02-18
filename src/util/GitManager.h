#pragma once

#include <string>
#include <optional>
#include <git2.h>

namespace application::util
{
	class GitManager
	{
	public:
		struct Identity
		{
			std::string mName = "";
			std::string mEmail = "";
			std::string mAccessToken = "";
		};

		struct OperationResult
		{
			bool mSuccess;
			std::string mMessage;
			uint8_t mToastType = -1;
		};

		static void Initialize();
		static void Shutdown();
		static Identity gUserIdentity;

		GitManager(const std::string& Path);
		~GitManager();

		OperationResult OpenRepository();
		OperationResult UpdateProject(const std::string& RemoteName = "origin", const std::string& Branch = "main");
		OperationResult PushProject(const std::string& RemoteName = "origin", const std::string& Branch = "main");

	private:
		git_repository* mRepo = nullptr;
		std::string mRepoPath;

		// Low-level operations
		OperationResult FetchLatest(const std::string& RemoteName = "origin");
		OperationResult MergeRemoteBranch(const std::string& Branch = "main");

		// Helper functions
		bool HasUncommittedChanges();
		bool IsBehindRemote(const std::string& Branch);
		bool IsAheadOfRemote(const std::string& Branch);
		bool StageAllChanges();
		bool CreateCommit(const std::string& Message);
		bool Push(const std::string& RemoteName, const std::string& Branch);
		bool StashChanges(git_oid* stash_id);
		bool PopStash();
		bool AbortMerge();

		static int CredentialsCallback(git_credential** out, const char* url,
			const char* username_from_url,
			unsigned int allowed_types, void* payload);
	};
}