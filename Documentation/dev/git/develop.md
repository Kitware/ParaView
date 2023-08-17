Develop ParaView with Git
=========================

This page documents how to develop ParaView using [GitLab][] and [Git][].
See the [README](README.md) for more information.

[GitLab]: https://gitlab.kitware.com/
[Git]: http://git-scm.com

Git is an extremely powerful version control tool that supports many
different "workflows" for individual development and collaboration.
Here we document procedures used by the ParaView development community.
In the interest of simplicity and brevity we do *not* provide an
explanation of why we use this approach.

Initial Setup
-------------

Before you begin, perform initial setup:

1.  Register [GitLab Access][] to create an account and select a user name.

2.  [Fork ParaView][] into your user's namespace on GitLab.

3.  Follow the [download instructions](download.md#clone) to create a
    local clone of the main ParaView repository.

        $ git clone --recursive https://gitlab.kitware.com/paraview/paraview.git ParaView

    The main repository will be configured as your `origin` remote.

4.  Run the [developer setup script][] to prepare your ParaView work tree and
    create Git command aliases used below:

        $ ./Utilities/SetupForDevelopment.sh

    This will prompt for your GitLab username and configure a remote
    called `gitlab` to refer to your fork. It will also setup a data directory for you.
    No need to do anything else.

[GitLab Access]: https://gitlab.kitware.com/users/sign_in
[Fork ParaView]: https://gitlab.kitware.com/paraview/paraview/-/forks/new
[developer setup script]: /Utilities/SetupForDevelopment.sh

Quick Start Guide
-----------------

This is a quick start guide so that you can start contributing to ParaView easily.
To understand the process more deeply, you can jump to the [workflow](#workflow)
section.

### Developement

Create a local branch for your changes:

```
git checkout -b your_branch
```

Make the needed changes in ParaView and use git locally to create logically separated commits.
There is no strict requirements regarding git commit messages syntax but a good rule of
thumb to follow is: `General domain: reason for change`, General domain being a class, a module
, a specific system like build or CI.

```
git commit -m "General domain: Short yet informative reason for the change"
```

Build ParaView following the [guide](Documentation/dev/build.md#) and fix any build warnings or issues that arise and seems related to your changes.

### Bringing in VTK changes

If you are working on a change that is across both VTK and ParaView, you can work directly on the VTK that is
inside your repository of ParaView. Just follow the steps in the [VTK develop guide](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/dev/git/develop.md#initial-setup)
starting at the usage of `SetupForDevelopment.sh` script.

You can then develop in the VTK directory as if it was any VTK repository, where you can make changes, commit and push to your VTK fork.

If you want to test the integration of your VTK changes in ParaView [continuous integration](continuous-integration), commit and push your changes
to your VTK fork, then commit the VTK submodule update with your changes in a separate commit in your ParaView branch, it will be found without issue.

Once your VTK changes are merged into VTK master, you can cleanup your history and commit the submodule update cleanly like this
(this will remove local non commited changes and also rebase your branch on the last master):

```
cd VTK
git fetch origin
git checkout master
git pull
cd ../
git fetch origin
git rebase -i origin/master # delete any VTK submodule update commit
git submodule update
git bump VTK master
```

### Testing

Every changes and new features needs to be tested. In ParaView, there are mainly two types of tests.
Python tests and XML tests. While both types of tests are as valid to add, XML tests should be preferred for standard
feature test when possible as they can be considered more generic.

#### XML Tests

To add a XML test, use the Tools -> Record Test menu in ParaView. Interact with ParaView in a way that test your feature
then record the XML test file.
You may need to edit this file slightly to make it work as expected, do not hesitate to look at other files.
Add the file to `Client/ParaView/Testing/XML` folder and list it in `Client/ParaView/Testing/XML/CMakeLists.txt`.
There are different categories of tests, but the safest bet is probably the `DATA_WITH_BASELINES` category.

You can then configure ParaView and run your test from the build directory and check that they pass:

```
cmake . && cmake --build .
ctest -VV -R yourTest
```

#### Python Tests

To add a Python test, write a pvpython script testing the feature, add it in `Client/ParaView/Testing/Python` folder
and list it in `Client/ParaView/Testing/Python/CMakeLists.txt`. There are different categories of tests,
but the safest bet is probably the `paraview_add_test_python` category.

```
cmake . && cmake --build .
ctest -VV -R yourTest
```

### Upload

Push your changes to the gitlab fork that you created in the [initial setup](#initial-setup) stage:

```
git push gitlab
```

### Data

If your test uses new data or baselines, you will need to add it to your fork.
For data, add the file names to the list in the corresponding `Clients/ParaView/Testing/*/CMakeLists.txt` and drop the files in `Testing/Data/`.
For baselines, just drop the file in `Clients/ParaView/Testing/Data/Baseline/` and run the following commands from your build directory:

```
cmake . && cmake --build .
```

This will transform your files into .sha512 files. Check your test is passing by running from your build directory:

```
ctest -VV -R yourTest
```

If it passes, add these .sha512 files and commit them, then push with:

```
git gitlab-push
```

### Create a Merge Request

Once you are happy with the state of your development on your fork, the next step is to create a merge request back into the main ParaView repository.

Open [](https://gitlab.kitware.com/username/paraview/-/merge_requests/new) in a browser, select your branch in the list and create a Merge Request against master.

In the description, write an informative explanation of your added features or bugfix. If there is an associated issue, link it with the `#number` in the description.

Tag some ParaView maintainers in the description to ensure someone will see it, eg: @cory.quammen, @ben.boeckel or @mwestphal.

### Robot Checks

Once the MR is created, our gitlab robot will check multiple things and make automated suggestions. Please read them and try to follow the instructions.
The two standard suggestions are related to formatting errors and adding markdown changelog.

To fix the formatting, just add a comment containing:

```
Do: reformat
```

Then, once the robot has fixed the formatting, fetch the changes locally (this will remove any local changes to your branch)

```
git fetch gitlab
git reset --hard gitlab/your_branch
```

To fix the changelog warning, create, add, commit and push a markdown (.md) file in `Documentation/release/dev` folder.
In this file, write a small markdown paragraph describing the development.
See other .md files in this folder for examples. It may look like this:

```
## Development title

A new feature that does this and that has been introduced.
This specific issue has been fixed in this particular way.
```

Suggestions and best practices on writing the changelog can be found in the `Documentation/release/dev/0-sample-topic.md` file.
This is an optional step but recommended to do for any new feature and user facing issues.

### Reviews

ParaView maintainers and developers will review your MR by leaving comments on it. Try to follow their instructions and be patient.
It can take a while to get a MR into mergeable form. This is a mandatory step, and it is absolutely normal to get change requests.

Review comments can be resolved, please resolve a comment once you've taken it into account and pushed related changes
or once you've reached an agreement with the commenter that nothing should be changed.

Once a reviewer is happy with your changes, they will add a `+X` comment. You need at least one `+2` or higher to consider
merging the MR. Two `+1`s do not equal a `+2`. If a reviewer leave a `-1` comment, please discuss with them to understand what is the issue and how it could be fixed.

Once you have pushed new changes, please tag reviewers again so that they can take a look.
If you do not tag reviewers, they may not know to revisit your changes. _Do not hesitate to tag them and ask for help_.

### Continuous Integration

Before merging a MR, the ParaView continuous integration (CI) needs to run and be green.
For CI to be functional, please read and follow this [guide](https://discourse.vtk.org/t/the-ultimate-how-to-make-ci-work-with-my-fork-guide/7581).
It was written for VTK but is as valid for ParaView.

To run the CI:
 - Click on the Pipelines Tab
 - Click on the last pipeline status badge
 - Press the `Play all manual` arrows on top of the Build and Test stages

Do not hesitate to tag a ParaView developer for help if needed.

You then need to wait for CI to run, it can take a while, up to a full day.

A successful CI should be fully green. If that is so, then your MR is ready !

If not, you need to analyze the issues and fix them. Recover the failure information this way:

Click on the pipelines tab, then on the last status badge, then on the `cdash-commit` job.
It will take you to the related CDash report where you will find all information.

Everything in the CDash report should be green except the `NotRun` and `Time` column. Take a look into each issue and fix them locally.
If there are issues in the pipeline but nothing is visible in the CDash, please ask a maintainer for help to figure out if anything should be done.
You can always try to rerun the failed job by clicking on the arrow of the job in the pipeline.

Once you have fixed some issues locally, commit and push them to gitlab, run the CI again and tag reviewers again for follow-up reviews.

### Merging

Once the MR has green CI and you have at least one `+2`, you can ask for a merge. Before that please make sure that:
 - Your commit history is logical (or squashed into a single commit) and cleaned up with good commit messages
 - You are rebased on a fairly recent version of master

If that is not the case, please rebase on master using the following commands:

```
git fetch origin
git rebase -i origin/master
git push gitlab -f
```

The interactive rebase will let you squash commits, reorganize commits and edit commit messages.

After the force push, make sure to run CI again.

Once all is done, tag a ParaView developer so that they can perform the merge command.

__Congratulations ! You just contributed to ParaView !__

Workflow
--------

ParaView development uses a [branchy workflow][] based on topic branches.
Our collaboration workflow consists of three main steps:

1.  Local Development:
    * [Update](#update)
    * [Create a Topic](#create-a-topic)

2.  Code Review (requires [GitLab Access][]):
    * [Share a Topic](#share-a-topic)
    * [Create a Merge Request](#create-a-merge-request)
    * [Review a Merge Request](#review-a-merge-request)
    * [Revise a Topic](#revise-a-topic)

3.  Integrate Changes:
    * [Merge a Topic](#merge-a-topic) (requires permission in GitLab)
    * [Delete a Topic](#delete-a-topic)

[branchy workflow]: http://public.kitware.com/Wiki/Git/Workflow/Topic

Update
------

1.  Update your local `master` branch:

        $ git checkout master
        $ git pullall
2.  Optionally push `master` to your fork in GitLab:

        $ git push gitlab master
    to keep it in sync.  The `git gitlab-push` script used to
    [Share a Topic](#share-a-topic) below will also do this.

Create a Topic
--------------

All new work must be committed on topic branches.
Name topics like you might name functions: concise but precise.
A reader should have a general idea of the feature or fix to be developed given
just the branch name. Additionally, it is preferred to have an issue associated with
every topic. The issue can document the bug or feature to be developed. In such
cases, being your topic name with the issue number.

1.  To start a new topic branch:

        $ git fetch origin

    If there is an issue associated with the topic, assign the issue to yourself
    using the "**Assignee**" field, and add the
    `workflow:active-development` label to it.

2.  For new development, start the topic from `origin/master`:

        $ git checkout -b my-topic origin/master

    For release branch fixes, start the topic from `origin/release`, and
    by convention use a topic name starting in `release-`:

        $ git checkout -b release-my-topic origin/release

    If subdmodules may have changed, the  run:

        $ git submodule update

3.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

    Caveats:
    * To add data follow these [vtk instructions][].
    * To add icons, Kitware's graphic designer may be able to help create an SVG icon.

    Commit messages must contain a brief description as the first line
    and a more detailed description of what the commit contains. If
    the commit contains a new feature, the detailed message must
    describe the new feature and why it is needed. If the commit
    contains a bug fix, the detailed message must describe the bug
    behavior, its underlying cause, and the approach to fix it. If the
    bug is described in the bug tracker, the commit message must
    contain a reference to the bug number.

4. Update a submodule if needed

    If you need to update a submodule (eg: VTK) in order to access a specific bugfix or features, first make sure
    that the needed developments have been merged into the main branch of the submodule. You can then use the command:

        $ git bump my-submodule my-hash-or-branch

    `my-submodule` being the submodule folder (eg: VTK), `my-hash-or-branch` being either a hash or a branch provided
    by any of your remote or your local repository, typically, `origin/master`.

    This will add a new commit which update the submodule and prefill the commit message with information about
    the different commits in the submodule. Make sure to still add some information about the reason for the bump.

    Please note you can run CI on a submodule commit in another remote, see [Continuous Integration] for more info.

4. Add some tests

    * Start `paraview.exe -dr` to ignore prefs (disable registry)
    * Choose `Tools .. Record Test` to start.
    * Choose `Tools .. Lock View Size Custom...` - a 400x400 window works well.
    * Perform actions in the GUI that exercise your feature. Stop recording.
    * Put the resulting XML file into `Clients/ParaView/Testing/XML`
    * Add it to CMakeLists.txt, probably in a TESTS_WITH_BASELINES section
        * you can manually add `<pqcompareview>` for multiple image comparisons, then add to the TESTS_WITH_INLINE_COMPARES section
    * Follow the [vtk instructions][] to add the baseline images, which live in `Testing/Data/Baseline/`.
        * Add your new baseline images to the list in `Testing/XML/CMakeLists.txt`
    * Add all testing files to your topic.

    Some background is in the [testing design wiki](https://www.paraview.org/Wiki/Testing_design).

5. Add release notes

    Notable changes should create a new file in `Documentation/release/dev/`
    named `my-topic.md` (replace `my-topic` with the name of your topic).
    This is not necessary for branches which are "trivial" such as fixing
    typos, updating test baselines, or are developer-oriented.

[vtk instructions]: https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/dev/git/data.md

Share a Topic
-------------

When a topic is ready for review and possible inclusion, share it by pushing
to a fork of your repository in GitLab.  Be sure you have registered and
signed in for [GitLab Access][] and created your fork by visiting the main
[ParaView GitLab][] repository page and using the "Fork" button in the upper right.

[ParaView GitLab]: https://gitlab.kitware.com/paraview/paraview

1.  Checkout the topic if it is not your current branch:

        $ git checkout my-topic

2.  Check what commits will be pushed to your fork in GitLab:

        $ git prepush

3.  Push commits in your topic branch to your fork in GitLab:

        $ git gitlab-push

    Notes:
    * If you are revising a previously pushed topic and have rewritten the
      topic history, add `-f` or `--force` to overwrite the destination.
    * If the topic adds data see [this note](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/dev/git/data.md#push).
    * The `gitlab-push` script also pushes the `master` branch to your
      fork in GitLab to keep it in sync with the upstream `master`.

    The output will include a link to the topic branch in your fork in GitLab
    and a link to a page for creating a Merge Request.

Create a Merge Request
----------------------

(If you already created a merge request for a given topic and have reached
this step after revising it, skip to the [next step](#review-a-merge-request).)

Visit your fork in GitLab, browse to the "**Merge Requests**" link on the
left, and use the "**New Merge Request**" button in the upper right to
reach the URL printed at the end of the [previous step](#share-a-topic).
It should be of the form:

    https://gitlab.kitware.com/<username>/paraview/-/merge_requests/new

Follow these steps:

1.  In the "**Source branch**" box select the `<username>/paraview` repository
    and the `my-topic` branch.

2.  In the "**Target branch**" box select the `paraview/paraview` repository and
    the `master` branch.  It should be the default.

    If your change is a fix for the `release` branch, you should still
    select the `master` branch as the target because the change needs
    to end up there too.

3.  Use the "**Compare branches**" button to proceed to the next page
    and fill out the merge request creation form.

4.  In the "**Title**" field provide a one-line summary of the entire
    topic.  This will become the title of the Merge Request.

    Example Merge Request Title:

        Wrapping: Add Java 1.x support

5.  In the "**Description**" field provide a high-level description
    of the change the topic makes and any relevant information about
    how to try it.
    *   Use `@username` syntax to draw attention of specific developers.
        This syntax may be used anywhere outside literal text and code
        blocks.  Or, wait until the [next step](#review-a-merge-request)
        and add comments to draw attention of developers.
    *   If your change is a fix for the `release` branch, indicate this
        so that a maintainer knows it should be merged to `release`.
    *   Optionally use a fenced code block with type `message` to specify
        text to be included in the generated merge commit message when the
        topic is [merged](#merge-a-topic).

    Example Merge Request Description:

        This branch requires Java 1.x which is not generally available yet.
        Get Java 1.x from ... in order to try these changes.

        ```message
        Add support for Java 1.x to the wrapping infrastructure.
        ```

        Cc: @user1 @user2

6.  The "**Assign to**", "**Milestone**", and "**Labels**" fields
    may be left blank.

7.  Use the "**Submit merge request**" button to create the merge request
    and visit its page.

Guidelines
----------
For guidelines with respect to licensing, naming conventions, code formatting,
documentation and further, please check [guidelines](guidelines.md)

Review a Merge Request
----------------------

Add comments mentioning specific developers using `@username` syntax to
draw their attention and have the topic reviewed.  After typing `@` and
some text, GitLab will offer completions for developers whose real names
or user names match.

Comments use [GitLab Flavored Markdown][] for formatting.  See GitLab
documentation on [Special GitLab References][] to add links to things
like merge requests and commits in other repositories.

When a merge request is ready for review, developers can use the
`triage:ready-for-review` to indicate the same to the reviewers. If reviewers
deems that it needs more work, they can add the `triage:needswork` label.
This can be repeated as many times as needed adding/removing labels as
appropriate.

If a merge request is waiting on dashboards, use the `triage:pending-dashboards`
label.

[GitLab Flavored Markdown]: https://gitlab.kitware.com/help/markdown/markdown
[Special GitLab References]: https://gitlab.kitware.com/help/markdown/markdown#special-gitlab-references

### Human Reviews ###

Reviewers may add comments providing feedback or to acknowledge their approval.

If a reviewer raises an issue via a thread, the contributor may resolve the thread if the
solution is indisputable. In case of cited lines within the issue, the
corresponding changes of the commit prior to a comment are shown. So it
is better to first push the change and then comment/resolve. When code is cited, the
last line is usually the line of interest, the above lines just provide the context.
Providing a comment just saying 'done' is unnecessary as GitLab
will indicate that the relevant code has changed.

It shall be a common goal for reviewers and contributors to limit the amount of generated emails.
Therefore reviewers are encouraged to comment systematic issues (e.g. missing `this->`)
only once but to indicate the general application. Contributors do not need to comment on trivial
fixes (e.g. typos in comments) but may simply solved the threads after fixing.

To raise the attention of individuals, e.g. a reviewer for another round of review, this person
can be pinged by addressing via `@` in a comment.

#### Special Comments #####

Lines of specific forms will be extracted during
[merging](#merge-a-topic) and included as trailing lines of the
generated merge commit message:

The *leading* line of a comment may optionally be exactly one of the
following votes followed by nothing but whitespace before the end
of the line:

* `-1` or `:-1:` indicates "the change is not ready for integration".
* `+1` or `:+1:` indicates "I like the change".
  This adds an `Acked-by:` trailer to the merge commit message.
* `+2` indicates "the change is ready for integration".
  This adds a `Reviewed-by:` trailer to the merge commit message.
* `+3` indicates "I have tested the change and verified it works".
  This adds a `Tested-by:` trailer to the merge commit message.

The middle lines of a comment may be free-form [GitLab Flavored Markdown][].

Zero or more *trailing* lines of a comment may each contain exactly one
of the following votes followed by nothing but whitespace before the end
of the line:

*   `Rejected-by: me` means "The change is not ready for integration."
*   `Acked-by: me` means "I like the change but defer to others."
*   `Reviewed-by: me` means "The change is ready for integration."
*   `Tested-by: me` means "I have tested the change and verified it works."

Each `me` reference may instead be an `@username` reference or a full
`Real Name <user@domain>` reference to credit someone else for performing
the review.  References to `me` and `@username` will automatically be
transformed into a real name and email address according to the user's
GitLab account profile.

#### Fetching Changes ####

One may fetch the changes associated with a merge request by using
the `git fetch` command line shown at the top of the Merge Request
page.  It is of the form:

    $ git fetch https://gitlab.kitware.com/$username/paraview.git $branch

This updates the local `FETCH_HEAD` to refer to the branch.

There are a few options for checking out the changes in a work tree:

*   One may checkout the branch:

        $ git checkout FETCH_HEAD -b $branch
    or checkout the commit without creating a local branch:

        $ git checkout FETCH_HEAD

*   Or, one may cherry-pick the commits to minimize rebuild time:

        $ git cherry-pick ..FETCH_HEAD

### Robot Reviews ###

The "Kitware Robot" automatically performs basic checks including clang-format on the commits
and adds a comment acknowledging or rejecting the topic.  This will be
repeated automatically whenever the topic is pushed to your fork again.

Automatic formatting can be triggered  by adding a comment with a single line.

    Do: reformat

A re-check may be explicitly requested by

    Do: check

A topic cannot be [merged](#merge-a-topic) until the automatic review
succeeds. It is necessary to correct the specific commit via rebase.

### Continuous Integration ###

ParaView uses [GitLab CI][] to test merge requests, configured by the top-level
`.gitlab-ci.yml` file.  Results may be seen both on the merge request's
pipeline page and on the [ParaView CDash Page][].  Filtered CDash results
showing just the pipeline's jobs can be reached by selecting the `cdash-commit`
job in the `External` stage of the pipeline. Note that due to GitLab changes,
the `External` stage may be in a separate pipeline for the same commit.

Lint build jobs run automatically after every push. Actual CI jobs require a
manual trigger to run:

* Merge request authors may visit their merge request's pipeline and click the
  "Play" button on one or more jobs manually.  If the merge request has the
  "Allow commits from members who can merge to the target branch" check box
  enabled, ParaView maintainers may use the "Play" button too.
  When in doubt, it's a good idea to run a few jobs as smoke tests to catch
  early build/test failures before a full CI run that would tie up useful resources.
  Note that, as detailed below, a full CI run is necessary before the request
  can be merged.

* When working simultaneously on ParaView and a submodule, e.g., VTK, it may be useful
  to run the CI before merging changes in the submodule in question. This is perfectly
  supported, just push your change on your remote for the submodule and update the submodule
  manually in a temporary commit, then run the CI. Make sure to remove this commit before the merge
  and use `git bump` when performing the actual submodule update.

* [ParaView GitLab Project Developers][] may trigger CI on a merge request by
  adding a comment with a command among the [comment trailing
  lines](#trailing-lines):

        Do: test

  `@kwrobot` will add an award emoji to the comment to indicate that it
  was processed and also trigger all manual jobs in the merge request's
  pipeline.

  The `Do: test` command accepts the following arguments:

  * `--named <regex>`, `-n <regex>`: Trigger jobs matching `<regex>` anywhere
    in their name.  Job names may be seen on the merge request's Pipelines tab.
  * `--stage <stage>`, `-s <stage>`: Only affect jobs in a given stage. Stage
    names may be seen on the merge request's Pipelines tab.  Note that the
    names are determined by what is in the `.gitlab-ci.yml` file and may be
    capitalized in the web page, so lowercasing the webpage's display name for
    stages may be required.
  * `--action <action>`, `-a <action>`: The action to perform on the jobs.
    Possible actions:

    * `manual` (the default): Start jobs awaiting manual interaction.
    * `unsuccessful`: Start or restart jobs which have not completed
      successfully.
    * `failed`: Restart jobs which have completed, but without success.
    * `completed`: Restart all completed jobs.

If the merge request topic branch is updated by a push, a new manual trigger
using one of the above methods is needed to start CI again. Currently running
jobs will generally be canceled automatically.

Before the merge, all the jobs must be run and reviewed, see below.

If you have any question about the CI process, do not hesitate to ask a CI maintainer:
 - ben.boeckel
 - mwestphal

[GitLab CI]: https://gitlab.kitware.com/help/ci/README.md
[ParaView CDash Page]: https://open.cdash.org/index.php?project=ParaView
[ParaView GitLab Project Developers]: https://gitlab.kitware.com/cmake/cmake/-/settings/members

### Reading CI Results ###

Reading CI results is a very important part of the merge request process
and is the responsibility of the author of the merge request, although reviewers
can usually help. There is two locations to read the results, GitLab CI and CDash.
Both should be checked and considered clean before merging.

To read GitLab CI result, click on the Pipelines tab then on the last pipeline.
It is expected to be fully green. If there is a yellow warning job, please consult CDash.
If there is a red failed job, click on it to see the reason for the failure.
It should clearly appears on the bottom of the log.
Possible failures are:
 - Timeouts: please rerun the job and report to CI maintainers
 - Memory related errors: please rerun the job and report to CI maintainers
 - Testing errors: please consult CDash for more information, usually an issue in your code
 - Non disclosed error: please consult CDash, usually a build error in your code

To read the CDash results, on the job page, click on the "cdash-commit" external job which
will open the commit-specific CDash page. Once it is open, make sure to show "All Build" at the bottom left of the page.
CDash results display error, warnings, and test failures for all the jobs.
It is expected to be green *except* for the "NoRun" and "Test Timings" categories, which can be ignored.

 - Configure warnings: there **must** not be any; to fix before the merge
 - Configure errors: there **must** not be any; to fix before the merge
 - Build warnings: there **must** not be any; to fix before the merge. If unrelated to your code, rerun the job and report to CI maintainers.
 - Build Errors: there **must** not be any; to fix before the merge. If unrelated to your code, report to CI maintainers.
 - NotRun test: ignore; these tests have self-diagnosed that they are not relevant on the testing machine.
 - Testing failure: there **should** not be any, ideally, to fix before the merge. If unrelated to your code, check the test history to see if it is a flaky test and report to CI maintainers.
 - Testing success: if your MR creates or modifies tests, please check that your test are listed there.
 - Test timings errors: can be ignored, but if it is all red, you may want to report it to CI maintainers.

To check the history of a failing test, on the test page, click on the "Summary" link to see a summary of the test for the day,
then click on the date controls on the top of the page to go back in time.
If the test fails on other MRs or on master, this is probably a flaky test, currently in the process of being fixed or excluded.
A flaky test can be ignored.

As a reminder, here is our current policy regarding CI results.
All the jobs must be run before merging.
Configure warnings and errors are not acceptable to merge and must be fixed.
Build warning and errors are not acceptable to merge and must be fixed.
Testing failure should be fixed before merging but can be accepted if a flaky test has been clearly identified.

Revise a Topic
--------------

If a topic is approved during GitLab review, skip to the
[next step](#merge-a-topic).  Otherwise, revise the topic
and push it back to GitLab for another review as follows:

1.  Checkout the topic if it is not your current branch:

        $ git checkout my-topic

2.  To revise the `3`rd commit back on the topic:

        $ git rebase -i HEAD~3

    (Substitute the correct number of commits back, as low as `1`.)
    Follow Git's interactive instructions.

3.  Return to the [above step](#share-a-topic) to share the revised topic.

Merge a Topic
-------------

Once review has concluded that the MR topic is ready for integration
(at least one `+2`), authorized developers may add a comment with a single
[*trailing* line](#trailing-lines):

    Do: merge

to ask that the change be merged into the upstream repository.  By
convention, do not request a merge if any `-1` or `Rejected-by:`
review comments have not been resolved and superseded by at least
`+1` or `Acked-by:` review comments from the same user.

Developers are encouraged to merge their own merge requests on review. However,
please do not merge unless you are available to address any dashboard issues that may
arise. Developers who repeatedly ignore dashboard issues following their merges may
loose developer privileges to the repository temporarily (or permanently)!

### Merge Success ###

If the merge succeeds the topic will appear in the upstream repository
`master` branch and the Merge Request will be closed automatically.
Any issues associated with the Merge Request will generally get closed
automatically. If not, the developer merging the changes should close such issues
and add a `workflow:customer-review` tag to the issue(s) addressed by the change.
Reporters and testers can then review the fix. Try to add enough information to
the Issue or the Merge Request to indicate how to test the functionality if not
obvious from the original Issue.

### Merge Failure ###

If the merge fails (likely due to a conflict), a comment will be added
describing the failure.  In the case of a conflict, fetch the latest
upstream history and rebase on it:

    $ git fetch origin
    $ git rebase origin/master

(If you are fixing a bug in the latest release then substitute
`origin/release` for `origin/master`.)

Return to the [above step](#share-a-topic) to share the revised topic.

Delete a Topic
--------------

After a topic has been merged upstream the Merge Request will be closed.
Now you may delete your copies of the branch.

1.  In the GitLab Merge Request page a "**Remove Source Branch**"
    button will appear.  Use it to delete the `my-topic` branch
    from your fork in GitLab.

2.  In your work tree checkout and update the `master` branch:

        $ git checkout master
        $ git pull

3.  Delete the local topic branch:

        $ git branch -d my-topic

    The `branch -d` command works only when the topic branch has been
    correctly merged.  Use `-D` instead of `-d` to force the deletion
    of an unmerged topic branch (warning - you could lose commits).

Contribute VTK Changes
----------------------

If you have any VTK changes, then you are required to get your changes
incorporated into VTK using [VTK's development workflow][]. Once your VTK topic has
been approved and merged into VTK, then:

1. Create a [ParaView topic](#create-a-topic) if you haven't already.
2. Add your VTK topic head (or the latest VTK
   origin/master which includes your VTK topic head).

        $ cd VTK
        $ git checkout master
        $ cd ..
        $ git add VTK
        $ git commit

3. Follow the merge process documented earlier.

[VTK's development workflow]: https://gitlab.kitware.com/vtk/vtk/-/tree/master/Documentation/dev/git
