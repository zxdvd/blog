```metadata
tags: shell, git
```

## git tips


### files

#### get staged files and pipe to lint tools like eslint
We often use git hooks to do some lint automatically before commit. It's simple to use a
 `npx eslint .` to check all files. However, this may take a long time, especially for
 large project. We can only lint staged files like folllowing:

    $ git diff --diff-filter=d --cached --name-only -z -- '*.js' \
        | xargs -0 -I % npx eslint --rulesdir ./script/eslint_rules %

The `-diff-filter=d` here will exclude deleted files (-D will only show deleted and -d is
 the opposed). The `-z` here is like `-print0` of find, it will use NUL as file separator
 so that this works well with files that contain spaces in filename.

#### repo backup and archive
You can use `git bundle` to backup a repo with all commit info and you can clone from
 the bundle file later.

    $ git bundle create backup.bundle master

You can tar all files of a repo using `git archive` but this contains only files and
 it doesn't contain any commit info. Of course, you cannot clone from it.

    $ git archive --format=tar.gz -o /tmp/repo.tar.gz --prefix=my-repo master

Now it's easy to understand the differences between `git bundle` and `git archive`.
You may wonder that why not simply `tar`?

Of course, you can `tar` a repo but `tar` deals with `directory` and it will `tar` unstaged
 files and ignored files which are not part of the repo. You can avoid this via `git bundle`.


### log related

#### filter commits
You can filter commits in one branch but not in another like following:

    $ git log branchA  ^branchB                   // in branchA but not in branchB
    $ git log branchA  ^branchB  ^branchC         // in branchA, not in branchB and branchC

You can filter commits by author and date like following:

    $ git lg --author xudong --after="2019-07-01" --until="2019-10-01" --no-merges

#### get date of latest commit
Sometimes, we may need latest commit date and inject it to config or version info in CI/CD.

    $ git log -1  --date=short --pretty=format:%cd     // 2019-10-20
    $ git log -1  --format=%aI master                  // 2019-10-10T13:09:42+08:00

### patch
Use `git diff` to create a patch and `git apply` to apply patch.

    $ git diff > unstaged.patch
    $ git diff --cached > staged.patch
    $ git apply staged.patch

### references
- [gist: pre-commit for eslint](https://gist.github.com/dahjelle/8ddedf0aebd488208a9a7c829f19b9e8)
