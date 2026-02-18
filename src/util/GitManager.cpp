#include <util/GitManager.h>
#include <util/Logger.h>
#include <ctime>
#include <chrono>

namespace application::util
{
	GitManager::Identity GitManager::gUserIdentity;

	void GitManager::Initialize()
	{
		git_libgit2_init();
	}

	void GitManager::Shutdown()
	{
		git_libgit2_shutdown();
	}

	GitManager::GitManager(const std::string& Path)
	{
		mRepoPath = Path;
	}

	GitManager::~GitManager()
	{
		if (mRepo)
		{
			git_repository_free(mRepo);
		}
	}

	GitManager::OperationResult GitManager::OpenRepository()
	{
		int error = git_repository_open(&mRepo, mRepoPath.c_str());
		if (error < 0)
		{
			const git_error* err = git_error_last();
			application::util::Logger::Error("GitManager", "Error opening repository: %s", err->message);
			return { false, ("Error opening repository: " + std::string(err->message)), 3 };
		}
		return { true, "Opened repository successful.", 1 };
	}

	GitManager::OperationResult GitManager::UpdateProject(const std::string& RemoteName, const std::string& Branch)
	{
		application::util::Logger::Info("GitManager", "Starting project update...");

		// Step 1: Fetch latest from remote
		auto fetchResult = FetchLatest(RemoteName);
		if (!fetchResult.mSuccess)
		{
			return fetchResult;
		}

		// Step 2: Check if we're behind remote
		bool behindRemote = IsBehindRemote(Branch);
		bool hasLocalChanges = HasUncommittedChanges();

		if (!behindRemote)
		{
			// No remote updates available
			if (hasLocalChanges)
			{
				application::util::Logger::Info("GitManager", "No remote updates. You have local changes that can be pushed.");
				return { true, "No remote updates available. You have local changes to push.", 2 };
			}
			else
			{
				application::util::Logger::Info("GitManager", "Already up-to-date");
				return { true, "Project is already up-to-date.", 1 };
			}
		}

		// Step 3: We have remote updates - check for local uncommitted changes
		git_oid stash_id;

		if (hasLocalChanges)
		{
			application::util::Logger::Info("GitManager", "Stashing local changes before merge");
			if (!StashChanges(&stash_id))
			{
				application::util::Logger::Error("GitManager", "Failed to stash local changes");
				return { false, "Failed to stash local changes.", 3 };
			}
		}

		// Step 4: Merge remote branch
		auto mergeResult = MergeRemoteBranch(Branch);

		if (!mergeResult.mSuccess)
		{
			application::util::Logger::Error("GitManager", "Merge failed, aborting and restoring local state");

			// Abort the merge
			AbortMerge();

			// Restore stashed changes if any
			if (hasLocalChanges)
			{
				PopStash();
			}

			return { false, "Update cancelled due to conflicts. Local files preserved.", 2 };
		}

		// Step 5: Restore stashed changes if any
		if (hasLocalChanges)
		{
			if (!PopStash())
			{
				application::util::Logger::Warning("GitManager", "Updated but failed to restore some local changes");
				return { true, "Project updated but some local changes may have been lost.", 2 };
			}

			application::util::Logger::Info("GitManager", "Project updated successfully, local changes restored");
			return { true, "Project updated successfully! Local changes restored.", 1 };
		}

		application::util::Logger::Info("GitManager", "Project updated successfully");
		return { true, "Project updated successfully!", 1 };
	}

	GitManager::OperationResult
		GitManager::PushProject(const std::string& RemoteName,
			const std::string& Branch)
	{
		Logger::Info("GitManager", "Starting push...");

		// ------------------------------------------------------------
		// 0) Validate identity
		// ------------------------------------------------------------
		if (gUserIdentity.mName.empty() || gUserIdentity.mEmail.empty())
		{
			return { false, "Please configure your name and email.", 3 };
		}

		// ------------------------------------------------------------
		// 1) Fetch first (CRITICAL)
		// ------------------------------------------------------------
		{
			auto fetchResult = FetchLatest(RemoteName);
			if (!fetchResult.mSuccess)
				return fetchResult;
		}

		// ------------------------------------------------------------
		// 2) Commit local changes if needed
		// ------------------------------------------------------------
		if (HasUncommittedChanges())
		{
			Logger::Info("GitManager", "Staging local changes...");

			if (!StageAllChanges())
				return { false, "Failed to stage changes.", 3 };

			std::time_t now = std::time(nullptr);
			char buf[64];
			std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S",
				std::localtime(&now));

			std::string message = "Auto-commit: " + std::string(buf);

			if (!CreateCommit(message))
				return { false, "Failed to create commit.", 3 };

			Logger::Info("GitManager", "Created commit: %s", message.c_str());
		}

		// ------------------------------------------------------------
		// 3) Are we ahead of the remote?
		// ------------------------------------------------------------
		bool hasRemoteBranch = false;
		{
			git_reference* remote_ref = nullptr;
			std::string remoteBranch = "refs/remotes/" + RemoteName + "/" + Branch;
			hasRemoteBranch = (git_reference_lookup(&remote_ref, mRepo,
				remoteBranch.c_str()) == 0);
			if (remote_ref) git_reference_free(remote_ref);
		}

		bool ahead = hasRemoteBranch ? IsAheadOfRemote(Branch) : true;

		if (!ahead)
		{
			Logger::Info("GitManager", "Nothing to push");
			return { true, "No changes to push.", 1 };
		}

		// ------------------------------------------------------------
		// 4) Push
		// ------------------------------------------------------------
		if (!Push(RemoteName, Branch))
		{
			return { false, "Failed to push to remote.", 3 };
		}

		Logger::Info("GitManager", "Push completed successfully");
		return { true, "Changes pushed successfully.", 1 };
	}


	GitManager::OperationResult GitManager::FetchLatest(const std::string& RemoteName)
	{
		git_remote* remote = nullptr;

		if (git_remote_lookup(&remote, mRepo, RemoteName.c_str()) < 0)
		{
			application::util::Logger::Error("GitManager", "Could not find remote: %s", RemoteName.c_str());
			return { false, "Could not find remote: " + RemoteName, 3 };
		}

		git_fetch_options FetchOpts = GIT_FETCH_OPTIONS_INIT;
		FetchOpts.callbacks.credentials = CredentialsCallback;
		FetchOpts.callbacks.payload = this;

		FetchOpts.callbacks.certificate_check = [](git_cert* cert, int valid, const char* host, void* payload) -> int {
			return 0;
			};

		if (git_remote_fetch(remote, nullptr, &FetchOpts, nullptr) < 0)
		{
			const git_error* err = git_error_last();
			application::util::Logger::Error("GitManager", "Fetch failed: %s", err->message);
			git_remote_free(remote);
			return { false, "Fetch failed: " + std::string(err->message), 3 };
		}

		git_remote_free(remote);
		return { true, "Fetch completed successfully.", 1 };
	}

	GitManager::OperationResult GitManager::MergeRemoteBranch(const std::string& Branch)
	{
		git_reference* remote_ref = nullptr;
		git_annotated_commit* their_head = nullptr;

		std::string remote_branch = "refs/remotes/origin/" + Branch;
		if (git_reference_lookup(&remote_ref, mRepo, remote_branch.c_str()) < 0)
		{
			return { false, "Could not lookup reference.", 3 };
		}

		if (git_annotated_commit_from_ref(&their_head, mRepo, remote_ref) < 0) {
			git_reference_free(remote_ref);
			return { false, "Could not get annotated commit from ref.", 3 };
		}

		// Perform merge
		git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
		git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
		checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

		const git_annotated_commit* their_heads[] = { their_head };

		if (git_merge(mRepo, their_heads, 1, &merge_opts, &checkout_opts) < 0) {
			const git_error* err = git_error_last();
			application::util::Logger::Error("GitManager", "Merge failed: %s", err->message);
			git_annotated_commit_free(their_head);
			git_reference_free(remote_ref);
			return { false, "Merge failed: " + std::string(err->message), 3 };
		}

		// Check if there are conflicts
		git_index* index = nullptr;
		git_repository_index(&index, mRepo);

		if (git_index_has_conflicts(index))
		{
			application::util::Logger::Error("GitManager", "Merge resulted in conflicts");
			git_index_free(index);
			git_annotated_commit_free(their_head);
			git_reference_free(remote_ref);
			return { false, "Merge resulted in conflicts.", 3 };
		}

		git_index_free(index);
		git_annotated_commit_free(their_head);
		git_reference_free(remote_ref);

		application::util::Logger::Info("GitManager", "Merge successful");
		return { true, "Merge successful.", 1 };
	}

	bool GitManager::HasUncommittedChanges()
	{
		application::util::Logger::Info("GitManager", "Checking for uncommitted changes, this can take a while for large projects");

		git_index* index = nullptr;
		if (git_repository_index(&index, mRepo) != 0)
			return false;

		if (git_index_has_conflicts(index))
		{
			git_index_free(index);
			return true;
		}

		git_tree* headTree = nullptr;
		git_object* headObj = nullptr;

		if (git_revparse_single(&headObj, mRepo, "HEAD^{tree}") == 0)
		{
			headTree = (git_tree*)headObj;

			git_diff* diff = nullptr;
			git_diff_options diffOpts = GIT_DIFF_OPTIONS_INIT;

			if (git_diff_tree_to_index(&diff, mRepo, headTree, index, &diffOpts) == 0)
			{
				if (git_diff_num_deltas(diff) > 0)
				{
					git_diff_free(diff);
					git_tree_free(headTree);
					git_index_free(index);
					return true;
				}
				git_diff_free(diff);
			}
			git_tree_free(headTree);
		}

		git_diff* wdDiff = nullptr;
		git_diff_options wdOpts = GIT_DIFF_OPTIONS_INIT;

		if (git_diff_index_to_workdir(&wdDiff, mRepo, index, &wdOpts) == 0)
		{
			if (git_diff_num_deltas(wdDiff) > 0)
			{
				git_diff_free(wdDiff);
				git_index_free(index);
				return true;
			}
			git_diff_free(wdDiff);
		}

		git_index_free(index);

		git_status_options statusOpts = GIT_STATUS_OPTIONS_INIT;
		statusOpts.show = GIT_STATUS_SHOW_WORKDIR_ONLY;
		statusOpts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;

		git_status_list* statusList = nullptr;
		if (git_status_list_new(&statusList, mRepo, &statusOpts) == 0)
		{
			bool hasUntracked = git_status_list_entrycount(statusList) > 0;
			git_status_list_free(statusList);
			return hasUntracked;
		}

		return false;
	}

	bool GitManager::IsBehindRemote(const std::string& Branch)
	{
		git_reference* local_ref = nullptr;
		git_reference* remote_ref = nullptr;
		size_t ahead, behind;

		std::string local_branch = "refs/heads/" + Branch;
		std::string remote_branch = "refs/remotes/origin/" + Branch;

		if (git_reference_lookup(&local_ref, mRepo, local_branch.c_str()) < 0)
		{
			return false;
		}

		if (git_reference_lookup(&remote_ref, mRepo, remote_branch.c_str()) < 0)
		{
			git_reference_free(local_ref);
			return false;
		}

		git_oid local_oid = *git_reference_target(local_ref);
		git_oid remote_oid = *git_reference_target(remote_ref);

		if (git_graph_ahead_behind(&ahead, &behind, mRepo, &local_oid, &remote_oid) < 0)
		{
			git_reference_free(local_ref);
			git_reference_free(remote_ref);
			return false;
		}

		git_reference_free(local_ref);
		git_reference_free(remote_ref);

		return behind > 0;
	}

	bool GitManager::IsAheadOfRemote(const std::string& Branch)
	{
		git_reference* local_ref = nullptr;
		git_reference* remote_ref = nullptr;
		size_t ahead, behind;

		std::string local_branch = "refs/heads/" + Branch;
		std::string remote_branch = "refs/remotes/origin/" + Branch;

		if (git_reference_lookup(&local_ref, mRepo, local_branch.c_str()) < 0)
		{
			return false;
		}

		if (git_reference_lookup(&remote_ref, mRepo, remote_branch.c_str()) < 0)
		{
			git_reference_free(local_ref);
			return false;
		}

		git_oid local_oid = *git_reference_target(local_ref);
		git_oid remote_oid = *git_reference_target(remote_ref);

		if (git_graph_ahead_behind(&ahead, &behind, mRepo, &local_oid, &remote_oid) < 0)
		{
			git_reference_free(local_ref);
			git_reference_free(remote_ref);
			return false;
		}

		git_reference_free(local_ref);
		git_reference_free(remote_ref);

		return ahead > 0;
	}

	bool GitManager::StageAllChanges()
	{
		git_index* index = nullptr;

		if (git_repository_index(&index, mRepo) < 0)
		{
			application::util::Logger::Error("GitManager", "Failed to get repository index");
			return false;
		}

		// Add all files (including untracked)
		if (git_index_add_all(index, nullptr, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr) < 0)
		{
			git_index_free(index);
			application::util::Logger::Error("GitManager", "Failed to stage changes");
			return false;
		}

		// Write the index
		if (git_index_write(index) < 0)
		{
			git_index_free(index);
			application::util::Logger::Error("GitManager", "Failed to write index");
			return false;
		}

		git_index_free(index);
		return true;
	}

	bool GitManager::CreateCommit(const std::string& Message)
	{
		git_signature* sig = nullptr;
		git_index* index = nullptr;
		git_oid tree_id, commit_id;
		git_tree* tree = nullptr;
		git_reference* head = nullptr;
		git_commit* parent = nullptr;

		// Create signature from configured identity
		if (git_signature_now(&sig, gUserIdentity.mName.c_str(), gUserIdentity.mEmail.c_str()) < 0)
		{
			application::util::Logger::Error("GitManager", "Failed to create signature");
			return false;
		}

		// Get the index
		if (git_repository_index(&index, mRepo) < 0)
		{
			git_signature_free(sig);
			return false;
		}

		// Write tree from index
		if (git_index_write_tree(&tree_id, index) < 0)
		{
			git_index_free(index);
			git_signature_free(sig);
			return false;
		}

		git_index_free(index);

		// Lookup tree
		if (git_tree_lookup(&tree, mRepo, &tree_id) < 0)
		{
			git_signature_free(sig);
			return false;
		}

		// Get HEAD reference and parent commit
		int parent_count = 0;
		const git_commit* parents[1];

		if (git_repository_head(&head, mRepo) == 0)
		{
			if (git_reference_peel((git_object**)&parent, head, GIT_OBJECT_COMMIT) == 0)
			{
				parents[0] = parent;
				parent_count = 1;
			}
			git_reference_free(head);
		}

		// Create commit
		if (git_commit_create_v(&commit_id, mRepo, "HEAD", sig, sig,
			nullptr, Message.c_str(), tree, parent_count, parent) < 0)
		{
			const git_error* err = git_error_last();
			application::util::Logger::Error("GitManager", "Failed to create commit: %s", err->message);

			if (parent) git_commit_free(parent);
			git_tree_free(tree);
			git_signature_free(sig);
			return false;
		}

		if (parent) git_commit_free(parent);
		git_tree_free(tree);
		git_signature_free(sig);

		return true;
	}

	static int PushUpdateCallback(
		const char* refname,
		const char* status,
		void* payload)
	{
		if (status)
		{
			application::util::Logger::Error(
				"GitManager",
				"Push rejected for %s: %s",
				refname,
				status
			);
		}
		else
		{
			application::util::Logger::Info(
				"GitManager",
				"Push updated %s successfully",
				refname
			);
		}
		return 0;
	}

	bool GitManager::Push(const std::string& RemoteName,
		const std::string& Branch)
	{
		git_remote* remote = nullptr;
		if (git_remote_lookup(&remote, mRepo, RemoteName.c_str()) < 0)
			return false;

		git_push_options opts = GIT_PUSH_OPTIONS_INIT;
		opts.callbacks.credentials = CredentialsCallback;
		opts.callbacks.payload = this;
		opts.callbacks.push_update_reference = PushUpdateCallback;

		std::string refspec =
			"refs/heads/" + Branch + ":refs/heads/" + Branch;

		const char* refspecs[] = { refspec.c_str() };
		git_strarray array = {
			const_cast<char**>(refspecs), 1
		};

		if (git_remote_push(remote, &array, &opts) < 0)
		{
			const git_error* err = git_error_last();
			Logger::Error("GitManager", "Push failed: %s", err->message);
			git_remote_free(remote);
			return false;
		}

		git_remote_free(remote);

		git_reference* local_ref = nullptr;
		if (git_branch_lookup(&local_ref, mRepo,
			Branch.c_str(), GIT_BRANCH_LOCAL) == 0)
		{
			git_branch_set_upstream(
				local_ref,
				(RemoteName + "/" + Branch).c_str()
			);
			git_reference_free(local_ref);
		}

		return true;
	}

	bool GitManager::StashChanges(git_oid* stash_id)
	{
		git_signature* sig = nullptr;

		if (git_signature_now(&sig, gUserIdentity.mName.c_str(), gUserIdentity.mEmail.c_str()) < 0)
		{
			application::util::Logger::Error("GitManager", "Failed to create signature for stash");
			return false;
		}

		int result = git_stash_save(stash_id, mRepo, sig, "Auto-stash before update", GIT_STASH_DEFAULT);

		git_signature_free(sig);

		if (result < 0)
		{
			const git_error* err = git_error_last();
			application::util::Logger::Error("GitManager", "Failed to stash: %s", err->message);
			return false;
		}

		return true;
	}

	bool GitManager::PopStash()
	{
		git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;

		// Apply the most recent stash (index 0)
		if (git_stash_pop(mRepo, 0, &opts) < 0)
		{
			const git_error* err = git_error_last();
			application::util::Logger::Error("GitManager", "Failed to pop stash: %s", err->message);
			return false;
		}

		return true;
	}

	bool GitManager::AbortMerge()
	{
		// Reset to HEAD to abort merge
		git_object* head_commit = nullptr;

		if (git_revparse_single(&head_commit, mRepo, "HEAD") < 0)
		{
			application::util::Logger::Error("GitManager", "Failed to get HEAD");
			return false;
		}

		git_checkout_options reset_opts = GIT_CHECKOUT_OPTIONS_INIT;
		reset_opts.checkout_strategy = GIT_CHECKOUT_FORCE;

		if (git_reset(mRepo, head_commit, GIT_RESET_HARD, &reset_opts) < 0)
		{
			const git_error* err = git_error_last();
			application::util::Logger::Error("GitManager", "Failed to abort merge: %s", err->message);
			git_object_free(head_commit);
			return false;
		}

		git_object_free(head_commit);

		// Clean up merge state
		git_repository_state_cleanup(mRepo);

		return true;
	}

	int GitManager::CredentialsCallback(git_credential** out, const char* url,
		const char* username_from_url,
		unsigned int allowed_types, void* payload)
	{
		GitManager* manager = static_cast<GitManager*>(payload);

		application::util::Logger::Info("GitManager", "Authenticating to: %s", url);

		if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT)
		{
			if (manager->gUserIdentity.mAccessToken.empty())
			{
				application::util::Logger::Error("GitManager", "No user identity configured");
				return -1;
			}

			return git_credential_userpass_plaintext_new(out,
				"x-access-token",
				manager->gUserIdentity.mAccessToken.c_str());
		}

		return -1;
	}
}