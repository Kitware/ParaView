Develop ParaView with Git
=========================

This page documents how to develop ParaView through [Git][].
See the [README](README.md) for more information.

[Git]: http://git-scm.com

Git is an extremely powerful version control tool that supports many
different "workflows" for individual development and collaboration.
Here we document procedures used by the ParaView development community.
In the interest of simplicity and brevity we do *not* provide an
explanation of why we use this approach.

Setup
-----

Before you begin, perform initial setup:

1.  Register [GitLab Access][] to create an account and select a user name.

2.  [Fork ParaView][] into your user's namespace on GitLab.

3.  Follow the [download instructions](download.md#clone) to create a
    local clone of the main ParaView repository.  Optionally configure
    Git to [use SSH instead of HTTPS](download.md#use-ssh-instead-of-https).
    Then clone:

        $ git clone --recursive https://gitlab.kitware.com/paraview/paraview.git ParaView
        $ cd ParaView
    The main repository will be configured as your `origin` remote.

4.  Run the [developer setup script][] to prepare your ParaView work tree and
    create Git command aliases used below:

        $ ./Utilities/SetupForDevelopment.sh
    This will prompt for your GitLab user name and configure a remote
    called `gitlab` to refer to it.

5.  (Optional but highly recommended.)
    [Register](https://open.cdash.org/register.php) with the ParaView project
    on Kitware's CDash instance to better know how your code performs in
    regression tests.  After registering and signing in, click on
    "All Dashboards" link in the upper left corner, scroll down and click
    "Subscribe to this project" on the right of ParaView.

[GitLab Access]: https://gitlab.kitware.com/users/sign_in
[Fork ParaView]: https://gitlab.kitware.com/paraview/paraview/-/forks/new
[developer setup script]: /Utilities/SetupForDevelopment.sh

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

Reviewers may add comments providing feedback or to acknowledge their
approval.  Lines of specific forms will be extracted during
[merging](#merge-a-topic) and included as trailing lines of the
generated merge commit message:

The *leading* line of a comment may optionally be exactly one of the
following votes followed by nothing but whitespace before the end
of the line:

*   `-1` or :-1: (`:-1:`) means "The change is not ready for integration."
*   `+1` or :+1: (`:+1:`) means "I like the change but defer to others."
*   `+2` means "The change is ready for integration."
*   `+3` means "I have tested the change and verified it works."

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

The "Kitware Robot" automatically performs basic checks on the commits
and adds a comment acknowledging or rejecting the topic.  This will be
repeated automatically whenever the topic is pushed to your fork again.
A re-check may be explicitly requested by adding a comment with a single
[*trailing* line](#trailing-lines):

    Do: check

A topic cannot be [merged](#merge-a-topic) until the automatic review
succeeds.

### Testing ###

ParaView uses [GitLab CI][] to test merge requests, configured by the top-level
`.gitlab-ci.yml` file.  Results may be seen both on the merge request's
pipeline page and on the [ParaView CDash Page][].  Filtered CDash results
showing just the pipeline's jobs can be reached by selecting the `cdash-commit`
job in the `External` stage of the pipeline. Note that due to GitLab changes,
the `External` stage may be in a separate pipeline for the same commit.

Lint build jobs run automatically after every push. Heavier jobs require a
manual trigger to run:

* Merge request authors may visit their merge request's pipeline and click the
  "Play" button on one or more jobs manually.  If the merge request has the
  "Allow commits from members who can merge to the target branch" check box
  enabled, ParaView maintainers may use the "Play" button too.

* [ParaView GitLab Project Developers][] may trigger CI on a merge request by
  adding a comment with a command among the [comment trailing
  lines](#trailing-lines):

        Do: test

  `@kwrobot` will add an award emoji to the comment to indicate that it
  was processed and also trigger all manual jobs in the merge request's
  pipeline.

  The `Do: test` command accepts the following arguments:

  * `--named <regex>`, `-n <regex>`: Trigger jobs matching `<regex>` anywhere
    in their name.  Job names may be seen on the merge request's pipeline page.
  * `--stage <stage>`, `-s <stage>`: Only affect jobs in a given stage. Stage
    names may be seen on the merge request's pipeline page.  Note that the
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

[GitLab CI]: https://gitlab.kitware.com/help/ci/README.md
[ParaView CDash Page]: https://open.cdash.org/index.php?project=ParaView
[ParaView GitLab Project Developers]: https://gitlab.kitware.com/cmake/cmake/-/settings/members

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

After a topic has been reviewed and approved in a GitLab Merge Request,
authorized developers may add a comment of the form

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
