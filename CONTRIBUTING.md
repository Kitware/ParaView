Contributing to ParaView
========================

This page documents at a very high level how to contribute to ParaView.
Please check our [developer instructions][] for a more detailed guide to
developing and contributing to the project, and our [ParaView Git README][]
for additional information.

The ParaView development cycle is built upon the following components:

1. [Issues][] identify any issues including bugs and feature requests. In
   general, every code change should have an associated issue which identifies
   the bug being fixed or the feature being added.

2. [Merge Requests][] are collections of changes that address issues.

3. [Labels][] are labels or tags that can be added and removed to/from issues
   and/or merge requests to annotate them including indicating their state in
   the development cycle. See [Labels Glossary][].

4. [Milestones][] refer to development milestones such as numbered public
   releases, or named internal releases.

Reporting Issues
================

If you have a bug report or a feature request for ParaView, you can use the
[issues][] tracker to report a [new issue][].

To report an issue.

1. Register [GitLab Access] to create an account and select a user name.
2. Create a [new issue][].
3. Ensure that the  issue has a **Title** and **Description**
   with enough details for someone from the development team to reproduce the
   issue. See [Gitlab Markdown] guide for styling the **Description**. Include
   screenshots and sample datasets whenever possible. Typically, reporter
   **should not** set any other fields for the issue, including
   **Assignee**, **Milestone**, or **Labels**. These get set by members of the
   development team.
4. If developers need more information on an issue, they will add the
   `triage:needinfo` label as add a comment for the reporter soliciting more
   information. Once the reporter has provided the necessary information, he/she
   should remove the `triage:needinfo` label from the issue to notify the
   development team.

When a developer starts working on an issue, the developer will add the
`workflow:active-development` label. Once the development is complete and the issue
resolved, the issue will be closed, and the `workflow:active-development` label
will be replaced by `workflow:customer-review`. At that point, the reporter can
checkout the latest `master` and confirm that the issue has been addressed. If so,
the reporter can remove the `workflow:customer-review` label. If the issue was not
addressed then the reporter should reopen the issue or solicit more information
from the developer by adding the `triage:needinfo` label.

To keep the number of open issues manageable, we will periodically expire old issues
with no activity. Such issues will be closed and tagged with the label
`triage:expired`. Such issues can be reopened if needed.

Notes for project managers
--------------------------

For every issue, project managers can assign:
1. `project:...` label to indicate which project this issue is necessary for. An issue
   may have multiple projects associated with it.
2. **Milestone** to indicate which release this issue fix is planned for.
3. `priority:...` label to indicate how critical is this issue for the specific
   milestone, ranging from `priority:required`, `priority:important`,
   `priority:nice-to-have`, and `priority:low`. Only one priority label makes
   sense at a time.

Notes for developers
--------------------

For every issue, developers can assign:
1. `area:...` labels to indicate which area this issue relates to e.g. `area:web`,
   `area:catalyst`, etc.
2. `triage:...` labels to indicate issue triage status. `triage:confirmed` is added
    when the issue has been confirmed. `triage:easy` is added for issues that are
    easy to fix. `triage:feature` is added to issues that are new feature requests.
    `triage:needinfo` is added to solicit more information from the reporter.
3. `triage:needinfo` label on closed issues means the reporter or reviewer is
    requesting more information from the developer before the fix can be reviewed.
    Please provide such information and then remove the label.
4. `workflow:active-development` label should be added to issues under development.


Fixing issues
=============

Typically, one addresses issues by writing code. To start contributing to ParaView:

1.  Register [GitLab Access] to create an account and select a user name.

2.  [Fork ParaView][] into your user's namespace on GitLab.

3.  Create a local clone of the main ParaView repository. Optionally configure
    Git to [use SSH instead of HTTPS][].
    Then clone:

        $ git clone --recursive https://gitlab.kitware.com/paraview/paraview.git ParaView
        $ cd ParaView
    The main repository will be configured as your `origin` remote.

    For more information see: [Setup][] and [download instructions][]

4.  Run the [developer setup script][] to prepare your ParaView work
    tree and create Git command aliases used below:

        $ ./Utilities/SetupForDevelopment.sh
    This will prompt for your GitLab user name and configure a remote
    called `gitlab` to refer to it. Choose the defaults for ParaView Data questions.

    For more information see: [Setup][]

5.  [Build Paraview][] and run it.

6.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

    Commit messages must be thorough and informative so that
    reviewers will have a good understanding of why the change is
    needed before looking at the code. Appropriately refer to the issue
    number, if applicable.

    For more information see: [Create a Topic][]

7.  Push commits in your topic branch to your fork in GitLab:

        $ git gitlab-push

    For more information see: [Share a Topic][]

8.  Run tests with ctest, or use the dashboard

9.  Visit your fork in GitLab, browse to the "**Merge Requests**" link on the
    left, and use the "**New Merge Request**" button in the upper right to
    create a Merge Request.

    For more information see: [Create a Merge Request][]

8.  Follow the [review][] process to get your merge request reviewed and tested.
    On success, the merge-request can be merged and closed.

    For more information see: [Review a Merge Request][]

9.  When a merge request is closed, any related issue should be closed (if not
    closed automatically) and assigned the `workflow:customer-review` label to
    request a review from the reporter.

10. Monitor the related issue for any `triage:needinfo` label additions to provide
    the customer with any details necessary to test the fix.

Our [Wiki][] is used to document features, flesh out designs and host other
documentation. We have a [ParaView Discourse][] forum to coordinate development
and to provide support.

[ParaView Git README]: Documentation/dev/git/README.md
[developer instructions]: Documentation/dev/git/develop.md
[GitLab Access]: https://gitlab.kitware.com/users/sign_in
[Fork ParaView]: https://gitlab.kitware.com/paraview/paraview/fork/new
[use SSH instead of HTTPS]: Documentation/dev/git/download.md#use-ssh-instead-of-https
[download instructions]: Documentation/dev/git/download.md#clone
[developer setup script]: /Utilities/SetupForDevelopment.sh
[Setup]: Documentation/dev/git/develop.md#Setup
[Build Paraview]: Documentation/dev/build.md
[Share a Topic]: Documentation/dev/git/develop.md#share-a-topic
[Create a Merge Request]: Documentation/dev/git/develop.md#create-a-merge-request
[Review a Merge Request]: Documentation/dev/git/develop.md#review-a-merge-request
[review]: Documentation/dev/git/develop.md#review-a-merge-request
[Issues]: https://gitlab.kitware.com/paraview/paraview/issues
[Merge Requests]: https://gitlab.kitware.com/paraview/paraview/merge_requests
[Labels]: https://gitlab.kitware.com/paraview/paraview/labels
[Milestones]: https://gitlab.kitware.com/paraview/paraview/milestones
[Wiki]: http://www.paraview.org/Wiki/ParaView
[ParaView Discourse]: https://discourse.paraview.org/
[Gitlab Markdown]: https://gitlab.kitware.com/help/markdown/markdown
[new issue]: https://gitlab.kitware.com/paraview/paraview/issues/new
[Labels Glossary]: Documentation/dev/git/labels.md
