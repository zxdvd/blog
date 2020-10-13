```metadata
tags: shell, git
```

## git advance: merge another repo and keep histories

We are trying mono repository recently and we want to merge our independent frontend,
 backend and mobile repositories to a single mono repository.

Of course, you can simply merge them via copying files but then you'll lose all commit
 histories. Is there any way to merge and keep the commit histories?

We can use `git pull repo1  master --allow-unrelated-histories` to merge an unrelated
 repository.

I've done a simple test:

- create a repo A with 3 commits that adds some files
- create another repo B with 3 commits that adds some files and there is a file has same
 name with A
- in repo B, added repo A as a remote: `git remote add repoA  ../gitrepoA`
- in repo B, pull repoA: `git pull repoA  master --allow-unrelated-histories`
- got conflict since repo A and repo B have some file with different contents. Resolve it
 and commit
- `git log` shows that all commits of repo A are merged in repo B with same commit hash

The final result:

```
❯ git lg
*   66f0af0 - (HEAD -> master) Merge branch 'master' of ../gitrepo1  (59 minutes ago) <Xudong Zhang>
|\
| * 223f41f - repo1 share  (62 minutes ago) <Xudong Zhang>
| * 9afe0cc - repo1 2  (62 minutes ago) <Xudong Zhang>
| * d1e4a2e - repo1 1  (62 minutes ago) <Xudong Zhang>
* bd756c3 - repo2 share  (60 minutes ago) <Xudong Zhang>
* 3c428f7 - repo2 2_2  (60 minutes ago) <Xudong Zhang>
* d9fbe32 - repo2 2_1  (60 minutes ago) <Xudong Zhang>
```

For our real world projects, we need to move our 3 repositories to the `packages` directory
 of new repository. Then we create directories like `packages/backend`, `packages/frontend`
 in old repositories and move all files to them and commit this change. Then we do the `pull`
 as above. All the 3 repositories are merged successfully.

### merge subdirectory of another repo
What about you only want to merge part of a repo? Of course, you can remove unwanted files
 and commit this change. But all these removed files are still kept in git history and waste
 spaces.

You can also split part of a repo to a new repo and then merge the new repo as above.

So how to split part of a repo?

    $ git subtree split -P packages/A -b temp_branch

You can use above command to split a subdirectory to a new branch. Later you can merge this
 subdirectory by pull this temp_branch.

Attention, this will rewrite commit log of course.



