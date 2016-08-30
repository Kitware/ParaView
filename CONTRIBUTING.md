Contributing to ParaView
========================

This page documents at a very high level how to contribute to ParaView.
Please check our [developer instructions][] for a more detailed guide to
developing and contributing to the project, and our [ParaView Git README][]
for additional information.

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

5.  [Build Paraview] and run it.

6.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

    Commit messages must be thorough and informative so that
    reviewers will have a good understanding of why the change is
    needed before looking at the code.

    For more information see: [Create a Topic][]

7.  Push commits in your topic branch to your fork in GitLab:

        $ git gitlab-push

    For more information see: [Share a Topic][]

8.  Run tests with ctest, or use the dashboard

9.  Visit your fork in GitLab, browse to the "**Merge Requests**" link on the
    left, and use the "**New Merge Request**" button in the upper right to
    create a Merge Request.

    For more information see: [Create a Merge Request][]


ParaView uses GitLab for code review and Buildbot to test proposed
patches before they are merged.

Our [Wiki][] is used to document features, flesh out designs and host other
documentation. We have several [Mailing Lists][] to coordinate development and
to provide support.

[ParaView Git README]: Documentation/dev/git/README.md
[developer instructions]: Documentation/dev/git/develop.md
[GitLab Access]: https://gitlab.kitware.com/users/sign_in
[Fork ParaView]: https://gitlab.kitware.com/paraview/paraview/fork/new
[use SSH instead of HTTPS]: Documentation/dev/git/download.md#use-ssh-instead-of-https
[download instructions]: Documentation/dev/git/download.md#clone
[developer setup script]: /Utilities/SetupForDevelopment.sh
[Setup]: Documentation/dev/git/develop.md#Setup
[Build Paraview]: http://www.paraview.org/Wiki/ParaView:Build_And_Install
[Create a Topic]: Documentation/dev/git/develop.md#create-a-topic
[Share a Topic]: Documentation/dev/git/develop.md#share-a-topic
[Create a Merge Request]: Documentation/dev/git/develop.md#create-a-merge-request


[Wiki]: http://www.paraview.org/Wiki/ParaView
[Mailing Lists]: http://www.paraview.org/mailing-lists/
